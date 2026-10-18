#include <editor_core/timeline.hpp>

#include "detail/state.hpp"

#include <climits>
#include <utility>

namespace editor_core::detail {

TimelineState::TimelineState(std::shared_ptr<ProfileState> profileState)
    : profile(std::move(profileState))
    , tractor(profile->native)
{}

} // namespace editor_core::detail

namespace editor_core {
using detail::requireMlt;

Timeline::Timeline(const Profile &profile)
    : state_(std::make_shared<detail::TimelineState>(profile.state_))
{}

Track Timeline::addTrack(TrackKind kind, std::string name)
{
    const int nativeIndex = state_->tractor.count();
    requireMlt(nativeIndex >= 0, ErrorCode::InvalidGraph, "MLT returned an invalid track count");
    auto track = std::make_shared<detail::TrackState>(
        state_->profile, kind, std::move(name), static_cast<std::size_t>(nativeIndex));
    requireMlt(state_->tractor.set_track(track->playlist, nativeIndex) == 0,
               ErrorCode::InvalidGraph, "MLT failed to add the track to the tractor");
    state_->tracks.push_back(track);
    state_->tractor.refresh();
    return Track(std::move(track));
}

void Timeline::setNativeTrack(Mlt::Producer &producer, int index)
{
    if (index < 0)
        throw Error(ErrorCode::InvalidArgument, "Native track index must be non-negative");
    requireMlt(producer.is_valid(), ErrorCode::InvalidGraph, "Cannot add an invalid MLT producer");
    requireMlt(state_->tractor.set_track(producer, index) == 0, ErrorCode::InvalidGraph,
               "MLT failed to set the native track");
    state_->tractor.refresh();
}

void Timeline::plantTransition(Mlt::Transition &transition, int trackA, int trackB)
{
    if (trackA < 0 || trackB < 0 || trackA == trackB || trackA >= state_->tractor.count()
        || trackB >= state_->tractor.count()) {
        throw Error(ErrorCode::InvalidArgument,
                    "Transition tracks must be distinct indices in the current MLT tractor");
    }
    requireMlt(transition.is_valid(), ErrorCode::InvalidGraph,
               "Cannot plant an invalid MLT transition");
    state_->tractor.plant_transition(transition, trackA, trackB);
    state_->tractor.refresh();
}

FrameCount Timeline::durationFrames() const
{
    state_->tractor.refresh();
    return state_->tractor.get_playtime();
}

Mlt::Tractor &Timeline::native() const
{
    return state_->tractor;
}

void Timeline::saveMltXml(const std::filesystem::path &path) const
{
    detail::writeGraphXml(state_->profile->native, state_->tractor, path);
}

RenderResult Timeline::render(const RenderSpec &spec) const
{
    const FrameCount frames = durationFrames();
    if (frames <= 0)
        throw Error(ErrorCode::InvalidGraph, "Cannot render an empty timeline");
    return detail::renderSnapshot(*state_->profile, state_->tractor, frames, spec);
}

} // namespace editor_core
