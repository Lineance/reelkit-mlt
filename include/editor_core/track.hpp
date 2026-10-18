#pragma once

#include <editor_core/profile.hpp>
#include <mlt++/MltFilter.h>
#include <mlt++/MltPlaylist.h>
#include <mlt++/MltProducer.h>
#include <mlt++/MltTransition.h>

#include <filesystem>
#include <memory>
#include <string>

namespace editor_core {
namespace detail {
struct TrackState;
}

class Track {
public:
    Track() = default;

    TrackKind kind() const;
    const std::string &name() const;
    std::size_t index() const;
    FrameCount durationFrames() const;

    // These calls mutate the same Mlt::Playlist returned by native().
    std::size_t append(Mlt::Producer &producer, FrameRange sourceRange);
    std::size_t appendFile(const std::filesystem::path &path, FrameRange sourceRange);
    void blank(FrameCount frames);
    void mixAdjacent(std::size_t firstClip, FrameCount duration, Mlt::Transition &transition);
    void dissolve(std::size_t firstClip, FrameCount duration);
    void attach(Mlt::Filter &filter);
    Mlt::Filter addText(const TextSpec &text);

    Mlt::Playlist &native() const;

private:
    explicit Track(std::shared_ptr<detail::TrackState> state);
    std::shared_ptr<detail::TrackState> state_;
    friend class Timeline;
};

} // namespace editor_core
