#include <editor_core/runtime.hpp>
#include <mlt++/MltFactory.h>

#include "detail/state.hpp"

#include <algorithm>
#include <mutex>

namespace editor_core::detail {

RuntimeState::RuntimeState(const RuntimeOptions &options)
    : moduleDirectory(options.moduleDirectory.lexically_normal().string())
    , repository(Mlt::Factory::init(moduleDirectory.empty() ? nullptr : moduleDirectory.c_str()))
{
    if (!repository)
        throw Error(ErrorCode::RuntimeInitialization, "MLT factory initialization failed");
    std::unique_ptr<Mlt::Properties> producers(repository->producers());
    if (!producers)
        throw Error(ErrorCode::RuntimeInitialization, "MLT service repository is unavailable");
}

RuntimeState::~RuntimeState()
{
    repository.reset();
    Mlt::Factory::close();
}

} // namespace editor_core::detail

namespace editor_core {
namespace {
std::mutex runtimeMutex;
std::weak_ptr<detail::RuntimeState> activeRuntime;

std::shared_ptr<detail::RuntimeState> acquireRuntime(const RuntimeOptions &options)
{
    const std::string moduleDirectory = options.moduleDirectory.lexically_normal().string();
    std::lock_guard<std::mutex> lock(runtimeMutex);
    if (auto existing = activeRuntime.lock()) {
        if (existing->moduleDirectory != moduleDirectory)
            throw Error(ErrorCode::RuntimeInitialization,
                        "MLT is already initialized with a different module directory");
        return existing;
    }

    auto state = std::make_shared<detail::RuntimeState>(options);
    activeRuntime = state;
    return state;
}

std::vector<std::string> namesFrom(Mlt::Properties *properties)
{
    std::unique_ptr<Mlt::Properties> owned(properties);
    std::vector<std::string> names;
    if (!owned)
        return names;

    names.reserve(static_cast<std::size_t>(owned->count()));
    for (int i = 0; i < owned->count(); ++i) {
        const char *name = owned->get_name(i);
        if (name != nullptr)
            names.emplace_back(name);
    }
    return names;
}

bool containsName(const std::vector<std::string> &names, const std::string &target)
{
    return std::find(names.begin(), names.end(), target) != names.end();
}
} // namespace

Runtime::Runtime(RuntimeOptions options)
    : state_(acquireRuntime(options))
{}

bool Runtime::hasProducer(const std::string &service) const
{
    return containsName(producers(), service);
}

bool Runtime::hasFilter(const std::string &service) const
{
    return containsName(filters(), service);
}

bool Runtime::hasTransition(const std::string &service) const
{
    return containsName(transitions(), service);
}

bool Runtime::hasConsumer(const std::string &service) const
{
    return containsName(consumers(), service);
}

std::vector<std::string> Runtime::producers() const
{
    return namesFrom(state_->repository->producers());
}

std::vector<std::string> Runtime::filters() const
{
    return namesFrom(state_->repository->filters());
}

std::vector<std::string> Runtime::transitions() const
{
    return namesFrom(state_->repository->transitions());
}

std::vector<std::string> Runtime::consumers() const
{
    return namesFrom(state_->repository->consumers());
}

Mlt::Repository &Runtime::nativeRepository() const
{
    return *state_->repository;
}

} // namespace editor_core
