#pragma once

#include <editor_core/editor_core.hpp>

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

std::string graphXmlBuffer(Mlt::Profile &profile, Mlt::Service &graph);
void writeGraphXml(Mlt::Profile &profile, Mlt::Service &graph,
                   const std::filesystem::path &path);
RenderResult renderProducer(ProfileState &profile, Mlt::Producer &producer,
                            FrameCount frames, const RenderSpec &spec);
RenderResult renderSnapshot(ProfileState &profile, Mlt::Service &graph,
                            FrameCount frames, const RenderSpec &spec);

void requireMlt(bool condition, ErrorCode code, const std::string &message);

} // namespace editor_core::detail
