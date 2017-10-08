//
//  Global.h
//  OpenJam
//
//  Created by Yevgeniy Logachev
//  Copyright (c) 2014 Yevgeniy Logachev. All rights reserved.
//
#ifndef VFSPP_GLOBAL_H
#define VFSPP_GLOBAL_H

#include <functional>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <list>
#include <string>
#include <sstream>
#include <algorithm>
#include <mutex>
#include <memory>
#include <utility>
#include <math.h>
#include <assert.h>
#include <fstream>

namespace vfspp
{
    
#define CLASS_PTR(_class) typedef std::shared_ptr<class _class> _class##Ptr;\
                          typedef std::weak_ptr<class _class> _class##Weak;
    
#if VFS_LOGS_ENABLED
#   define VFS_LOG(...) printf(__VA_ARGS__)
#else
#   define VFS_LOG(...)
#endif

}; // namespace vfspp

#endif // VFSPP_GLOBAL_H
