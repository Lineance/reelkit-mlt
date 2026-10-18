#include <editor_core/track.hpp>

#include "detail/state.hpp"

#include <algorithm>
#include <climits>
#include <utility>

namespace editor_core::detail {

TrackState::TrackState(std::shared_ptr<ProfileState> profileState, TrackKind trackKind,
                       std::string trackName, std::size_t trackIndex)
    : profile(std::move(profileState))
    , kind(trackKind)
    , name(std::move(trackName))
    , index(trackIndex)
    , playlist(profile->native)
{}

} // namespace editor_core::detail

namespace editor_core {
using detail::requireMlt;

Track::Track(std::shared_ptr<detail::TrackState> state)
    : state_(std::move(state))
{}

TrackKind Track::kind() const
{
    requireMlt(state_ != nullptr, ErrorCode::InvalidGraph, "Track handle is empty");
    return state_->kind;
}

const std::string &Track::name() const
{
    requireMlt(state_ != nullptr, ErrorCode::InvalidGraph, "Track handle is empty");
    return state_->name;
}

std::size_t Track::index() const
{
    requireMlt(state_ != nullptr, ErrorCode::InvalidGraph, "Track handle is empty");
    return state_->index;
}

FrameCount Track::durationFrames() const
{
    requireMlt(state_ != nullptr, ErrorCode::InvalidGraph, "Track handle is empty");
    return state_->playlist.get_playtime();
}

std::size_t Track::append(Mlt::Producer &producer, FrameRange sourceRange)
{
    requireMlt(state_ != nullptr, ErrorCode::InvalidGraph, "Track handle is empty");
    requireMlt(producer.is_valid(), ErrorCode::InvalidGraph, "Cannot append an invalid MLT producer");
    const int result = state_->playlist.append(producer, sourceRange.mltIn(), sourceRange.mltOut());
    requireMlt(result == 0, ErrorCode::InvalidGraph, "MLT failed to append the producer");
    return static_cast<std::size_t>(std::max(0, state_->playlist.count() - 1));
}

std::size_t Track::appendFile(const std::filesystem::path &path, FrameRange sourceRange)
{
    requireMlt(state_ != nullptr, ErrorCode::InvalidGraph, "Track handle is empty");
    const std::string resource = path.string();
    Mlt::Producer producer(state_->profile->native, resource.c_str());
    requireMlt(producer.is_valid(), ErrorCode::InvalidGraph,
               "MLT could not create a producer for " + resource);
    return append(producer, sourceRange);
}

void Track::blank(FrameCount frames)
{
    requireMlt(state_ != nullptr, ErrorCode::InvalidGraph, "Track handle is empty");
    if (frames <= 0 || frames > INT_MAX)
        throw Error(ErrorCode::InvalidArgument, "Blank duration must be a positive frame count");
    const int result = state_->playlist.blank(static_cast<int>(frames - 1));
    requireMlt(result == 0, ErrorCode::InvalidGraph, "MLT failed to append a blank");
}

void Track::mixAdjacent(std::size_t firstClip, FrameCount duration, Mlt::Transition &transition)
{
    requireMlt(state_ != nullptr, ErrorCode::InvalidGraph, "Track handle is empty");
    if (duration <= 0 || duration > INT_MAX)
        throw Error(ErrorCode::InvalidArgument, "Transition duration must be a positive frame count");
    const int result = state_->playlist.mix(static_cast<int>(firstClip), static_cast<int>(duration),
                                             &transition);
    requireMlt(result == 0, ErrorCode::InvalidGraph, "MLT failed to mix adjacent clips");
}

void Track::dissolve(std::size_t firstClip, FrameCount duration)
{
    requireMlt(state_ != nullptr, ErrorCode::InvalidGraph, "Track handle is empty");
    Mlt::Transition luma(state_->profile->native, "luma");
    requireMlt(luma.is_valid(), ErrorCode::MissingService, "MLT luma transition is unavailable");
    mixAdjacent(firstClip, duration, luma);
}

void Track::attach(Mlt::Filter &filter)
{
    requireMlt(state_ != nullptr, ErrorCode::InvalidGraph, "Track handle is empty");
    requireMlt(filter.is_valid(), ErrorCode::InvalidGraph, "Cannot attach an invalid MLT filter");
    requireMlt(state_->playlist.attach(filter) == 0, ErrorCode::InvalidGraph,
               "MLT failed to attach the filter");
}

Mlt::Filter Track::addText(const TextSpec &text)
{
    requireMlt(state_ != nullptr, ErrorCode::InvalidGraph, "Track handle is empty");
    if (text.text.empty() || text.size <= 0 || text.opacity < 0.0 || text.opacity > 1.0)
        throw Error(ErrorCode::InvalidArgument, "Invalid qtext properties");
    const FrameCount duration = state_->playlist.get_playtime();
    if (duration <= 0)
        throw Error(ErrorCode::InvalidGraph, "Add a clip before attaching a timed text filter");

    Mlt::Filter filter(state_->profile->native, "qtext", "");
    requireMlt(filter.is_valid(), ErrorCode::MissingService, "MLT qtext filter is unavailable");
    filter.set("argument", text.text.c_str());
    filter.set("family", text.family.c_str());
    filter.set("fgcolour", text.foreground.c_str());
    filter.set("bgcolour", text.background.c_str());
    filter.set("olcolour", text.outlineColor.c_str());
    filter.set("size", text.size);
    filter.set("weight", text.weight);
    filter.set("outline", text.outline);
    filter.set("pad", text.padding);
    filter.set("opacity", text.opacity);
    filter.set_in_and_out(0, static_cast<int>(duration - 1));
    attach(filter);
    return Mlt::Filter(filter);
}

Mlt::Playlist &Track::native() const
{
    requireMlt(state_ != nullptr, ErrorCode::InvalidGraph, "Track handle is empty");
    return state_->playlist;
}

} // namespace editor_core
