#pragma once

#include <editor_core/runtime.hpp>
#include <mlt++/MltProfile.h>

#include <memory>

namespace editor_core {
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

} // namespace editor_core
