#ifndef THREADINGPOLICY_HPP
#define THREADINGPOLICY_HPP

#include <mutex>

#include "Global.h"

namespace vfspp
{

struct MultiThreadedPolicy {
    static std::lock_guard<std::mutex> Lock(std::mutex& m) noexcept { return std::lock_guard(m); }
};

struct SingleThreadedPolicy {
    struct DummyLock {};
    static DummyLock Lock(std::mutex&) noexcept { return {}; }
};

} // namespace vfspp

#endif // THREADINGPOLICY_HPP