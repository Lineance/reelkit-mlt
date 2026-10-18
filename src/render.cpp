#include <editor_core/render.hpp>
#include <mlt++/MltConsumer.h>

#include "detail/state.hpp"

#include <filesystem>
#include <utility>

namespace editor_core::detail {

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

RenderResult renderProducer(ProfileState &profile, Mlt::Producer &producer,
                            FrameCount frames, const RenderSpec &spec)
{
    if (spec.target.empty())
        throw Error(ErrorCode::InvalidArgument, "Render target path is empty");
    ensureOutputDirectory(spec.target);

    Mlt::Consumer consumer(profile.native, "avformat", spec.target.c_str());
    requireMlt(consumer.is_valid(), ErrorCode::MissingService,
               "MLT avformat consumer is unavailable");
    consumer.set("real_time", -1);
    consumer.set("terminate_on_pause", 1);
    consumer.set("an", spec.enableAudio ? 0 : 1);
    consumer.set("audio_off", spec.enableAudio ? 0 : 1);
    consumer.set("vn", spec.enableVideo ? 0 : 1);

    if (!spec.muxer.empty())
        consumer.set("f", spec.muxer.c_str());
    if (spec.enableVideo) {
        if (!spec.videoCodec.empty())
            consumer.set("vcodec", spec.videoCodec.c_str());
        if (!spec.pixelFormat.empty())
            consumer.set("pix_fmt", spec.pixelFormat.c_str());
        if (spec.quality >= 0)
            consumer.set("crf", spec.quality);
        if (!spec.preset.empty())
            consumer.set("preset", spec.preset.c_str());
    }
    if (spec.enableAudio) {
        if (!spec.audioCodec.empty())
            consumer.set("acodec", spec.audioCodec.c_str());
        if (!spec.mltAudioFormat.empty())
            consumer.set("mlt_audio_format", spec.mltAudioFormat.c_str());
    }
    for (const auto &[key, value] : spec.consumerProperties)
        consumer.set(key.c_str(), value.c_str());

    requireMlt(consumer.connect(producer) == 0, ErrorCode::RenderFailure,
               "Could not connect graph to MLT avformat consumer");
    requireMlt(consumer.run() == 0, ErrorCode::RenderFailure, "MLT avformat render failed");
    requireMlt(std::filesystem::is_regular_file(spec.target)
                   && std::filesystem::file_size(spec.target) > 0,
               ErrorCode::RenderFailure, "MLT render did not create an output file");

    RenderResult result;
    result.output = spec.target;
    result.frames = frames;
    result.frameRate = profile.settings.frameRate;
    result.durationSeconds = static_cast<double>(frames) * result.frameRate.denominator
                             / result.frameRate.numerator;
    if (spec.muxer == "wav" && spec.audioCodec == "pcm_s16le") {
        result.warnings.emplace_back(
            "PCM16 WAV level behavior is not validated with the current MLT runtime");
    }
    return result;
}

RenderResult renderSnapshot(ProfileState &profile, Mlt::Service &graph,
                           FrameCount frames, const RenderSpec &spec)
{
    const std::string xml = graphXmlBuffer(profile.native, graph);
    Mlt::Producer snapshot(profile.native, "xml-string", xml.c_str());
    requireMlt(snapshot.is_valid(), ErrorCode::SerializationFailure,
               "MLT could not reload an in-memory graph snapshot");
    return renderProducer(profile, snapshot, frames, spec);
}

} // namespace editor_core::detail
