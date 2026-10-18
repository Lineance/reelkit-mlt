#pragma once

#include <editor_core/types.hpp>
#include <mlt++/MltRepository.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace editor_core {
namespace detail {
struct RuntimeState;
}

struct RuntimeOptions {
    std::filesystem::path moduleDirectory;
};

class Runtime {
public:
    explicit Runtime(RuntimeOptions options = {});

    bool hasProducer(const std::string &service) const;
    bool hasFilter(const std::string &service) const;
    bool hasTransition(const std::string &service) const;
    bool hasConsumer(const std::string &service) const;
    std::vector<std::string> producers() const;
    std::vector<std::string> filters() const;
    std::vector<std::string> transitions() const;
    std::vector<std::string> consumers() const;
    Mlt::Repository &nativeRepository() const;

private:
    std::shared_ptr<detail::RuntimeState> state_;
    friend class Profile;
};

} // namespace editor_core
