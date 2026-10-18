#pragma once

#include <editor_core/track.hpp>
#include <mlt++/MltTractor.h>

#include <memory>
#include <string>
#include <vector>

namespace editor_core {
namespace detail {
struct TimelineState;
}
struct RenderSpec;
struct RenderResult;

class Timeline {
public:
    explicit Timeline(const Profile &profile);
    Timeline(const Timeline &) = delete;
    Timeline &operator=(const Timeline &) = delete;
    Timeline(Timeline &&) noexcept = default;
    Timeline &operator=(Timeline &&) noexcept = default;

    Track addTrack(TrackKind kind = TrackKind::Video, std::string name = {});
    void setNativeTrack(Mlt::Producer &producer, int index);
    void plantTransition(Mlt::Transition &transition, int trackA, int trackB);
    FrameCount durationFrames() const;

    Mlt::Tractor &native() const;
    void saveMltXml(const std::filesystem::path &path) const;
    RenderResult render(const RenderSpec &spec) const;

private:
    std::shared_ptr<detail::TimelineState> state_;
};

} // namespace editor_core
