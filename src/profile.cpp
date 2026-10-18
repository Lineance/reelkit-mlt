#include <editor_core/profile.hpp>

#include "detail/state.hpp"

namespace editor_core::detail {

ProfileState::ProfileState(std::shared_ptr<RuntimeState> runtimeState, ProfileSpec profileSettings)
    : runtime(std::move(runtimeState))
    , settings(std::move(profileSettings))
    , native()
{
    if (!runtime)
        throw Error(ErrorCode::RuntimeInitialization, "Profile requires an active MLT Runtime");
    if (settings.width <= 0 || settings.height <= 0 || settings.frameRate.numerator <= 0
        || settings.frameRate.denominator <= 0 || settings.sampleAspectNumerator <= 0
        || settings.sampleAspectDenominator <= 0 || settings.displayAspectNumerator <= 0
        || settings.displayAspectDenominator <= 0) {
        throw Error(ErrorCode::InvalidArgument, "Invalid MLT profile dimensions or ratios");
    }

    native.set_width(settings.width);
    native.set_height(settings.height);
    native.set_frame_rate(settings.frameRate.numerator, settings.frameRate.denominator);
    native.set_sample_aspect(settings.sampleAspectNumerator, settings.sampleAspectDenominator);
    native.set_display_aspect(settings.displayAspectNumerator, settings.displayAspectDenominator);
    native.set_progressive(settings.progressive ? 1 : 0);
    native.set_explicit(1);
    if (settings.colorspace)
        native.set_colorspace(*settings.colorspace);
    if (!native.is_valid())
        throw Error(ErrorCode::InvalidArgument, "MLT rejected the requested profile");
}

} // namespace editor_core::detail

namespace editor_core {

Profile::Profile(const Runtime &runtime, ProfileSpec settings)
    : state_(std::make_shared<detail::ProfileState>(runtime.state_, std::move(settings)))
{}

Mlt::Profile &Profile::native() const
{
    return state_->native;
}

const ProfileSpec &Profile::settings() const
{
    return state_->settings;
}

double Profile::fps() const
{
    return state_->native.fps();
}

} // namespace editor_core
