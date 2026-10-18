#pragma once

#include <editor_core/editor_core.hpp>
#include <mlt++/MltConsumer.h>

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace editor_core::detail {

struct RuntimeState {
    explicit RuntimeState(const RuntimeOptions &options);
    ~RuntimeState();

    RuntimeState(const RuntimeState &) = delete;
    RuntimeState &operator=(const RuntimeState &) = delete;

    std::string moduleDirectory;
    std::unique_ptr<Mlt::Repository> repository;
};

struct ProfileState {
    ProfileState(std::shared_ptr<RuntimeState> runtimeState, ProfileSpec profileSettings);

    std::shared_ptr<RuntimeState> runtime;
    ProfileSpec settings;
    Mlt::Profile native;
};

struct TrackState {
    TrackState(std::shared_ptr<ProfileState> profileState, TrackKind trackKind,
               std::string trackName, std::size_t trackIndex);

    std::shared_ptr<ProfileState> profile;
    TrackKind kind;
    std::string name;
    std::size_t index;
    Mlt::Playlist playlist;
};

struct TimelineState {
    explicit TimelineState(std::shared_ptr<ProfileState> profileState);

    std::shared_ptr<ProfileState> profile;
    Mlt::Tractor tractor;
    std::vector<std::shared_ptr<TrackState>> tracks;
};

struct MltDocumentState {
    MltDocumentState(std::shared_ptr<ProfileState> profileState, Mlt::Producer rootProducer);

    std::shared_ptr<ProfileState> profile;
    Mlt::Producer producer;
};

struct RenderJobState {
    RenderJobState(std::shared_ptr<ProfileState> profileState, const std::string &xml,
                   FrameCount frameCount, RenderSpec renderSpec);
    ~RenderJobState();

    void start();
    void cancel();
    bool isRunning() const;
    RenderProgress progress() const;
    RenderResult wait();

    std::shared_ptr<ProfileState> profile;
    Mlt::Producer snapshot;
    RenderSpec spec;
    FrameCount totalFrames;
    std::unique_ptr<Mlt::Consumer> consumer;
    std::atomic<bool> started{false};
    std::atomic<bool> cancelled{false};
    std::atomic<bool> completed{false};
};

std::string graphXmlBuffer(Mlt::Profile &profile, Mlt::Service &graph);
void writeGraphXml(Mlt::Profile &profile, Mlt::Service &graph,
                   const std::filesystem::path &path);
std::unique_ptr<RenderJobState> createRenderJob(std::shared_ptr<ProfileState> profile,
                                                Mlt::Service &graph,
                                                FrameCount frames,
                                                const RenderSpec &spec);
void requireMlt(bool condition, ErrorCode code, const std::string &message);

} // namespace editor_core::detail
