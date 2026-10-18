#include <reelkit/mlt/render.hpp>

#include <climits>
#include <utility>

namespace reelkit::mlt::detail {

void requireMlt(bool condition, ErrorCode code, const std::string &message)
{
    if (!condition)
        throw Error(code, message);
}

} // namespace reelkit::mlt::detail

namespace reelkit::mlt {

Error::Error(ErrorCode code, std::string message)
    : std::runtime_error(std::move(message))
    , code_(code)
{}

double RationalRate::asDouble() const
{
    if (numerator <= 0 || denominator <= 0)
        throw Error(ErrorCode::InvalidArgument, "Frame rate numerator and denominator must be positive");
    return static_cast<double>(numerator) / static_cast<double>(denominator);
}

FrameRange::FrameRange(FrameCount startFrame, FrameCount endFrame)
    : start(startFrame)
    , end(endFrame)
{
    if (start < 0 || end <= start || end - 1 > INT_MAX)
        throw Error(ErrorCode::InvalidArgument,
                    "FrameRange must be non-empty, non-negative, and fit MLT's frame index");
}

int FrameRange::mltIn() const
{
    return static_cast<int>(start);
}

int FrameRange::mltOut() const
{
    return static_cast<int>(end - 1);
}

RenderSpec RenderSpec::mp4(std::filesystem::path target)
{
    RenderSpec spec;
    spec.target = std::move(target);
    spec.muxer = "mp4";
    spec.videoCodec = "libx264";
    spec.audioCodec = "aac";
    spec.pixelFormat = "yuv420p";
    spec.mltAudioFormat = "f32le";
    spec.preset = "ultrafast";
    spec.quality = 23;
    return spec;
}

RenderSpec RenderSpec::m4a(std::filesystem::path target)
{
    RenderSpec spec;
    spec.target = std::move(target);
    spec.muxer = "ipod";
    spec.enableVideo = false;
    spec.audioCodec = "aac";
    spec.mltAudioFormat = "f32le";
    return spec;
}

RenderSpec RenderSpec::floatWav(std::filesystem::path target)
{
    RenderSpec spec;
    spec.target = std::move(target);
    spec.muxer = "wav";
    spec.enableVideo = false;
    spec.audioCodec = "pcm_f32le";
    spec.mltAudioFormat = "f32le";
    return spec;
}

} // namespace reelkit::mlt
