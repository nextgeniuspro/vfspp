//
//  CStringUtils.h
//  OpenJam
//
//  Created by Yevgeniy Logachev on 11/19/14.
//  Copyright (c) 2014 yev. All rights reserved.
//

#ifndef CSTRINGUTILS_H
#define CSTRINGUTILS_H

#include "Global.h"

namespace vfspp
{

class CStringUtils
{
public:
    static void Split(std::vector<std::string>& tokens, const std::string& text, char delimeter);
    static std::string Replace(std::string string, const std::string& search, const std::string& replace);
    
    static bool EndsWith(const std::string& fullString, const std::string& ending);
    static bool StartsWith(const std::string& fullString, const std::string& starting);
};

}; // namespace vfspp

#endif /* defined(CSTRINGUTILS_H) */
