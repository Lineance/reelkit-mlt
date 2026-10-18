#pragma once

#include <editor_core/timeline.hpp>
#include <mlt++/MltProducer.h>

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace editor_core {

struct RenderSpec {
    std::filesystem::path target;
    std::string muxer;
    std::string videoCodec{"libx264"};
    std::string audioCodec{"aac"};
    std::string pixelFormat{"yuv420p"};
    std::string mltAudioFormat{"f32le"};
    std::string preset{"ultrafast"};
    int quality{23};
    bool enableVideo{true};
    bool enableAudio{true};
    std::map<std::string, std::string> consumerProperties;

    static RenderSpec mp4(std::filesystem::path target);
    static RenderSpec m4a(std::filesystem::path target);
    static RenderSpec floatWav(std::filesystem::path target);
};

struct RenderResult {
    std::filesystem::path output;
    FrameCount frames{0};
    RationalRate frameRate{};
    double durationSeconds{0.0};
    bool cancelled{false};
    std::vector<std::string> warnings;
};

struct RenderProgress {
    FrameCount completedFrames{0};
    FrameCount totalFrames{0};
    double fraction{0.0};
};

namespace detail {
struct RenderJobState;
struct MltDocumentState;
}

class RenderJob {
public:
    RenderJob(const RenderJob &) = delete;
    RenderJob &operator=(const RenderJob &) = delete;
    RenderJob(RenderJob &&) noexcept;
    RenderJob &operator=(RenderJob &&) noexcept;
    ~RenderJob();

    void start();
    void cancel();
    bool isRunning() const;
    RenderProgress progress() const;
    RenderResult wait();

private:
    explicit RenderJob(std::unique_ptr<detail::RenderJobState> state);
    std::unique_ptr<detail::RenderJobState> state_;
    friend class Timeline;
    friend class MltDocument;
};

class MltDocument {
public:
    static MltDocument load(const Profile &profile, const std::filesystem::path &path);

    MltDocument(const MltDocument &) = delete;
    MltDocument &operator=(const MltDocument &) = delete;
    MltDocument(MltDocument &&) noexcept = default;
    MltDocument &operator=(MltDocument &&) noexcept = default;

    Mlt::Producer &native() const;
    FrameCount durationFrames() const;
    void saveMltXml(const std::filesystem::path &path) const;
    RenderJob startRender(const RenderSpec &spec) const;
    RenderResult render(const RenderSpec &spec) const;

private:
    explicit MltDocument(std::shared_ptr<detail::MltDocumentState> state);
    std::shared_ptr<detail::MltDocumentState> state_;
};

} // namespace editor_core
