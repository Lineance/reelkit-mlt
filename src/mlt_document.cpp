#include <reelkit/mlt/render.hpp>

#include "detail/state.hpp"

#include <filesystem>
#include <utility>
#include <utility>

namespace reelkit::mlt::detail {

MltDocumentState::MltDocumentState(std::shared_ptr<ProfileState> profileState,
                                   Mlt::Producer rootProducer)
    : profile(std::move(profileState))
    , producer(rootProducer)
{}

} // namespace reelkit::mlt::detail

namespace reelkit::mlt {

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

RenderJob MltDocument::startRender(const RenderSpec &spec) const
{
    return RenderJob(detail::createRenderJob(state_->profile, state_->producer,
                                             durationFrames(), spec));
}

RenderResult MltDocument::render(const RenderSpec &spec) const
{
    auto job = startRender(spec);
    job.start();
    return job.wait();
}

} // namespace reelkit::mlt
