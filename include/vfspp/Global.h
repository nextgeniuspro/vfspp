#ifndef GLOBAL_H
#define GLOBAL_H

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
#include <iostream>
#include <filesystem>

#ifdef VFSPP_ENABLE_MULTITHREADING
#define VFSPP_MT_SUPPORT_ENABLED true
#else
#define VFSPP_MT_SUPPORT_ENABLED false
#endif

#endif // GLOBAL_H