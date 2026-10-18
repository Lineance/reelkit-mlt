#pragma once

#include <stdexcept>
#include <string>
#include <cstdint>
#include <optional>

namespace reelkit::mlt {

enum class ErrorCode {
    InvalidArgument,
    RuntimeInitialization,
    MissingService,
    InvalidGraph,
    SerializationFailure,
    RenderFailure,
    UnsupportedOutput,
};

class Error final : public std::runtime_error {
public:
    Error(ErrorCode code, std::string message);
    ErrorCode code() const noexcept { return code_; }

private:
    ErrorCode code_;
};

struct RationalRate {
    int numerator{30};
    int denominator{1};

    double asDouble() const;
};

using FrameCount = std::int64_t;

// Public ranges are zero-based and half-open; MLT's inclusive out frame is an implementation detail.
struct FrameRange {
    FrameCount start{0};
    FrameCount end{0};

    FrameRange(FrameCount startFrame, FrameCount endFrame);
    FrameCount duration() const noexcept { return end - start; }
    int mltIn() const;
    int mltOut() const;
};

struct ProfileSpec {
    int width{1920};
    int height{1080};
    RationalRate frameRate{};
    int sampleAspectNumerator{1};
    int sampleAspectDenominator{1};
    int displayAspectNumerator{16};
    int displayAspectDenominator{9};
    bool progressive{true};
    std::optional<int> colorspace;
};

enum class TrackKind {
    Video,
    Audio,
    Overlay,
    Mixed,
};

struct TextSpec {
    std::string text;
    std::string family{"Sans"};
    std::string foreground{"0xffffffff"};
    std::string background{"0x00000000"};
    std::string outlineColor{"0x00000000"};
    int size{48};
    int weight{400};
    int outline{0};
    int padding{0};
    double opacity{1.0};
};

} // namespace reelkit::mlt
