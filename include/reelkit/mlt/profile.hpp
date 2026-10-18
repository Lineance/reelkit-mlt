#pragma once

#include <reelkit/mlt/runtime.hpp>
#include <mlt++/MltProfile.h>

#include <memory>

namespace reelkit::mlt {
namespace detail {
struct ProfileState;
}

class Profile {
public:
    explicit Profile(const Runtime &runtime, ProfileSpec settings = {});

    Mlt::Profile &native() const;
    const ProfileSpec &settings() const;
    double fps() const;

private:
    std::shared_ptr<detail::ProfileState> state_;
    friend class Track;
    friend class Timeline;
    friend class MltDocument;
};

} // namespace reelkit::mlt
