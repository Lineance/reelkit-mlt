#include <editor_core/render.hpp>

#include "detail/state.hpp"

#include <filesystem>
#include <utility>
#include <utility>

namespace editor_core::detail {

MltDocumentState::MltDocumentState(std::shared_ptr<ProfileState> profileState,
                                   Mlt::Producer rootProducer)
    : profile(std::move(profileState))
    , producer(rootProducer)
{}

} // namespace editor_core::detail

namespace editor_core {

MltDocument::MltDocument(std::shared_ptr<detail::MltDocumentState> state)
    : state_(std::move(state))
{}

MltDocument MltDocument::load(const Profile &profile, const std::filesystem::path &path)
{
    if (path.empty())
        throw Error(ErrorCode::InvalidArgument, "MLT XML source path is empty");
    const std::string resource = path.string();
    Mlt::Producer producer(profile.native(), resource.c_str());
    detail::requireMlt(producer.is_valid(), ErrorCode::SerializationFailure,
                       "MLT could not load document " + resource);
    return MltDocument(std::make_shared<detail::MltDocumentState>(profile.state_, producer));
}

Mlt::Producer &MltDocument::native() const
{
    return state_->producer;
}

FrameCount MltDocument::durationFrames() const
{
    return state_->producer.get_playtime();
}

void MltDocument::saveMltXml(const std::filesystem::path &path) const
{
    detail::writeGraphXml(state_->profile->native, state_->producer, path);
}

RenderResult MltDocument::render(const RenderSpec &spec) const
{
    const FrameCount frames = durationFrames();
    if (frames <= 0)
        throw Error(ErrorCode::InvalidGraph, "Cannot render an empty MLT document");
    return detail::renderSnapshot(*state_->profile, state_->producer, frames, spec);
}

} // namespace editor_core
