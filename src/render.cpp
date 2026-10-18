#include <reelkit/mlt/render.hpp>

#include "detail/state.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <thread>
#include <utility>

namespace reelkit::mlt::detail {
namespace {
void ensureOutputDirectory(const std::filesystem::path &path)
{
    const auto parent = path.parent_path();
    if (!parent.empty())
        std::filesystem::create_directories(parent);
}
} // namespace

void writeGraphXml(Mlt::Profile &profile, Mlt::Service &graph,
                   const std::filesystem::path &path)
{
    if (path.empty())
        throw Error(ErrorCode::InvalidArgument, "MLT XML target path is empty");
    ensureOutputDirectory(path);
    Mlt::Consumer xml(profile, "xml", path.c_str());
    requireMlt(xml.is_valid(), ErrorCode::SerializationFailure, "MLT XML consumer is unavailable");
    requireMlt(xml.connect(graph) == 0, ErrorCode::SerializationFailure,
               "Could not connect MLT graph to XML consumer");
    requireMlt(xml.run() == 0, ErrorCode::SerializationFailure, "MLT XML serialization failed");
    requireMlt(std::filesystem::is_regular_file(path) && std::filesystem::file_size(path) > 0,
               ErrorCode::SerializationFailure, "MLT XML output was not created");
}

std::string graphXmlBuffer(Mlt::Profile &profile, Mlt::Service &graph)
{
    Mlt::Consumer xml(profile, "xml", "buffer");
    requireMlt(xml.is_valid(), ErrorCode::SerializationFailure, "MLT XML consumer is unavailable");
    requireMlt(xml.connect(graph) == 0, ErrorCode::SerializationFailure,
               "Could not connect MLT graph to XML consumer");
    requireMlt(xml.run() == 0, ErrorCode::SerializationFailure, "MLT XML graph snapshot failed");
    const char *buffer = xml.get("buffer");
    requireMlt(buffer != nullptr && *buffer != '\0', ErrorCode::SerializationFailure,
               "MLT XML consumer returned an empty graph snapshot");
    return buffer;
}

RenderJobState::RenderJobState(std::shared_ptr<ProfileState> profileState,
                               const std::string &xml,
                               FrameCount frameCount,
                               RenderSpec renderSpec)
    : profile(std::move(profileState))
    , snapshot(profile->native, "xml-string", xml.c_str())
    , spec(std::move(renderSpec))
    , totalFrames(frameCount)
{
    if (spec.target.empty())
        throw Error(ErrorCode::InvalidArgument, "Render target path is empty");
    ensureOutputDirectory(spec.target);
    requireMlt(snapshot.is_valid(), ErrorCode::SerializationFailure,
               "MLT could not reload an in-memory graph snapshot");

    consumer = std::make_unique<Mlt::Consumer>(profile->native, "avformat", spec.target.c_str());
    requireMlt(consumer->is_valid(), ErrorCode::MissingService,
               "MLT avformat consumer is unavailable");
    consumer->set("real_time", -1);
    consumer->set("terminate_on_pause", 1);
    consumer->set("an", spec.enableAudio ? 0 : 1);
    consumer->set("audio_off", spec.enableAudio ? 0 : 1);
    consumer->set("vn", spec.enableVideo ? 0 : 1);

    if (!spec.muxer.empty())
        consumer->set("f", spec.muxer.c_str());
    if (spec.enableVideo) {
        if (!spec.videoCodec.empty())
            consumer->set("vcodec", spec.videoCodec.c_str());
        if (!spec.pixelFormat.empty())
            consumer->set("pix_fmt", spec.pixelFormat.c_str());
        if (spec.quality >= 0)
            consumer->set("crf", spec.quality);
        if (!spec.preset.empty())
            consumer->set("preset", spec.preset.c_str());
    }
    if (spec.enableAudio) {
        if (!spec.audioCodec.empty())
            consumer->set("acodec", spec.audioCodec.c_str());
        if (!spec.mltAudioFormat.empty())
            consumer->set("mlt_audio_format", spec.mltAudioFormat.c_str());
    }
    for (const auto &[key, value] : spec.consumerProperties)
        consumer->set(key.c_str(), value.c_str());

    requireMlt(consumer->connect(snapshot) == 0, ErrorCode::RenderFailure,
               "Could not connect graph snapshot to MLT avformat consumer");
}

RenderJobState::~RenderJobState()
{
    if (consumer && started.load() && !consumer->is_stopped()) {
        cancelled.store(true);
        consumer->stop();
    }
}

void RenderJobState::start()
{
    if (cancelled.load())
        throw Error(ErrorCode::InvalidGraph, "Cannot start a cancelled RenderJob");
    bool expected = false;
    if (!started.compare_exchange_strong(expected, true))
        throw Error(ErrorCode::InvalidGraph, "RenderJob can only be started once");
    const int status = consumer->start();
    if (status != 0) {
        started.store(false);
        throw Error(ErrorCode::RenderFailure, "MLT avformat consumer failed to start");
    }
}

void RenderJobState::cancel()
{
    cancelled.store(true);
    if (started.load() && consumer && !consumer->is_stopped())
        consumer->stop();
    completed.store(true);
}

bool RenderJobState::isRunning() const
{
    return started.load() && !completed.load() && consumer && !consumer->is_stopped();
}

RenderProgress RenderJobState::progress() const
{
    RenderProgress result;
    result.totalFrames = totalFrames;
    if (!consumer || !started.load())
        return result;
    if (completed.load()) {
        result.completedFrames = cancelled.load() ? 0 : totalFrames;
    } else {
        const int position = consumer->position();
        result.completedFrames = std::clamp<FrameCount>(position, 0, totalFrames);
    }
    if (result.totalFrames > 0)
        result.fraction = static_cast<double>(result.completedFrames)
                          / static_cast<double>(result.totalFrames);
    return result;
}

RenderResult RenderJobState::wait()
{
    if (!started.load() && cancelled.load()) {
        completed.store(true);
        RenderResult cancelledResult;
        cancelledResult.output = spec.target;
        cancelledResult.frames = totalFrames;
        cancelledResult.frameRate = profile->settings.frameRate;
        cancelledResult.durationSeconds = static_cast<double>(totalFrames)
                                          * cancelledResult.frameRate.denominator
                                          / cancelledResult.frameRate.numerator;
        cancelledResult.cancelled = true;
        cancelledResult.warnings.emplace_back("Render was cancelled before it started");
        return cancelledResult;
    }
    if (!started.load())
        start();
    while (!consumer->is_stopped())
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    completed.store(true);

    RenderResult result;
    result.output = spec.target;
    result.frames = totalFrames;
    result.frameRate = profile->settings.frameRate;
    result.durationSeconds = static_cast<double>(totalFrames) * result.frameRate.denominator
                             / result.frameRate.numerator;
    result.cancelled = cancelled.load();
    if (result.cancelled) {
        result.warnings.emplace_back("Render was cancelled; the output may be partial");
        return result;
    }
    requireMlt(std::filesystem::is_regular_file(spec.target)
                   && std::filesystem::file_size(spec.target) > 0,
               ErrorCode::RenderFailure, "MLT render did not create an output file");
    if (spec.muxer == "wav" && spec.audioCodec == "pcm_s16le") {
        result.warnings.emplace_back(
            "PCM16 WAV level behavior is not validated with the current MLT runtime");
    }
    return result;
}

std::unique_ptr<RenderJobState> createRenderJob(std::shared_ptr<ProfileState> profile,
                                                Mlt::Service &graph,
                                                FrameCount frames,
                                                const RenderSpec &spec)
{
    if (frames <= 0)
        throw Error(ErrorCode::InvalidGraph, "Cannot render an empty MLT graph");
    const std::string xml = graphXmlBuffer(profile->native, graph);
    return std::make_unique<RenderJobState>(std::move(profile), xml, frames, spec);
}

} // namespace reelkit::mlt::detail

namespace reelkit::mlt {

RenderJob::RenderJob(std::unique_ptr<detail::RenderJobState> state)
    : state_(std::move(state))
{}

RenderJob::RenderJob(RenderJob &&) noexcept = default;
RenderJob &RenderJob::operator=(RenderJob &&) noexcept = default;
RenderJob::~RenderJob() = default;

void RenderJob::start()
{
    if (!state_)
        throw Error(ErrorCode::InvalidGraph, "RenderJob is empty");
    state_->start();
}

void RenderJob::cancel()
{
    if (state_)
        state_->cancel();
}

bool RenderJob::isRunning() const
{
    return state_ && state_->isRunning();
}

RenderProgress RenderJob::progress() const
{
    if (!state_)
        return {};
    return state_->progress();
}

RenderResult RenderJob::wait()
{
    if (!state_)
        throw Error(ErrorCode::InvalidGraph, "RenderJob is empty");
    return state_->wait();
}

} // namespace reelkit::mlt
