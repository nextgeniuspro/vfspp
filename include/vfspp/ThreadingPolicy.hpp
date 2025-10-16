#ifndef VFSPP_THREADINGPOLICY_HPP
#define VFSPP_THREADINGPOLICY_HPP

#include <mutex>

#include "Global.h"

namespace vfspp
{

/*
* Use this policy for multi-threaded applications
*/
struct MultiThreadedPolicy {
    static std::lock_guard<std::mutex> Lock(std::mutex& m) noexcept { return std::lock_guard(m); }
};


/*
* Use this policy for single-threaded applications
*/
struct SingleThreadedPolicy {
    struct DummyLock {};
    static DummyLock Lock(std::mutex&) noexcept { return {}; }
};

} // namespace vfspp

#endif // VFSPP_THREADINGPOLICY_HPP