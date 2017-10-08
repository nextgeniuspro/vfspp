//
//  CFileInfo.cpp
//  vfspp
//
//  Created by Yevgeniy Logachev on 6/23/16.
//
//

#include "CFileInfo.h"
#include "CStringUtilsVFS.h"

using namespace vfspp;

// *****************************************************************************
// Constants
// *****************************************************************************

// *****************************************************************************
// Public Methods
// *****************************************************************************

CFileInfo::CFileInfo()
{
    
}

CFileInfo::~CFileInfo()
{
    
}

CFileInfo::CFileInfo(const std::string& basePath, const std::string& fileName, bool isDir)
{
    Initialize(basePath, fileName, isDir);
}

CFileInfo::CFileInfo(const std::string& filePath)
{
    std::size_t found = filePath.rfind("/");
    if (found != std::string::npos)
    {
        const std::string basePath = filePath.substr(0, found + 1);
        std::string fileName;
        if (found != filePath.length())
        {
            fileName = filePath.substr(found + 1, filePath.length() - found - 1);
        }
        
        Initialize(basePath, fileName, false);
    }
}

void CFileInfo::Initialize(const std::string& basePath, const std::string& fileName, bool isDir)
{
    m_BasePath = basePath;
    m_Name = fileName;
    m_IsDir = isDir;
    
    if (!CStringUtils::EndsWith(m_BasePath, "/"))
    {
        m_BasePath += "/";
    }
    
    if (isDir && !CStringUtils::EndsWith(m_Name, "/"))
    {
        m_Name += "/";
    }
    
    if (CStringUtils::StartsWith(m_Name, "/"))
    {
        m_Name = m_Name.substr(1, m_Name.length() - 1);
    }
    
    m_AbsolutePath = m_BasePath + m_Name;
    
    size_t dotsNum = std::count(m_Name.begin(), m_Name.end(), '.');
    bool isDotOrDotDot = (dotsNum == m_Name.length() && isDir);
    
    if (!isDotOrDotDot)
    {
        std::size_t found = m_Name.rfind(".");
        if (found != std::string::npos)
        {
            m_BaseName = m_Name.substr(0, found);
            if (found < m_Name.length())
            {
                m_Extension = m_Name.substr(found, m_Name.length() - found);
            }
        }
    }
}

const std::string& CFileInfo::Name() const
{
    return m_Name;
}

const std::string& CFileInfo::BaseName() const
{
    return m_BaseName;
}

const std::string& CFileInfo::Extension() const
{
    return m_Extension;
}

const std::string& CFileInfo::AbsolutePath() const
{
    return m_AbsolutePath;
}

const std::string& CFileInfo::BasePath() const
{
    return m_BasePath;
}

bool CFileInfo::IsDir() const
{
    return m_IsDir;
}

bool CFileInfo::IsValid() const
{
    return !m_AbsolutePath.empty();
}

// *****************************************************************************
// Protected Methods
// *****************************************************************************

// *****************************************************************************
// Private Methods
// *****************************************************************************
