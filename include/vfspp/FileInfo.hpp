#ifndef FILEINFO_HPP
#define FILEINFO_HPP

#include "Global.h"
#include "StringUtils.hpp"

namespace vfspp
{
    
class FileInfo final
{
public:
    FileInfo(const std::string& filePath)
    {
        std::size_t found = filePath.rfind("/");
        if (found != std::string::npos) {
            const std::string basePath = filePath.substr(0, found + 1);
            std::string fileName;
            if (found != filePath.length()) {
                fileName = filePath.substr(found + 1, filePath.length() - found - 1);
            }
            
            Configure(basePath, fileName, false);
        }
    }

    FileInfo(const std::string& basePath, const std::string& fileName, bool isDir)
    {
        Configure(basePath, fileName, isDir);
    }

    FileInfo() = delete;

    ~FileInfo()
    {

    }
    
    /*
     * Get file name with extension
     */
    inline const std::string& Name() const
    {
        return m_Name;
    }
    
    /*
     * Get file name without extension
     */
    inline const std::string& BaseName() const
    {
        return m_BaseName;
    }
    
    /*
     * Get file extension
     */
    inline const std::string& Extension() const
    {
        return m_Extension;
    }
    
    /*
     * Get absolute file path
     */
    inline const std::string& AbsolutePath() const
    {
        return m_AbsolutePath;
    }
    
    /*
     * Directory where file is located
     */
    inline const std::string& BasePath() const
    {
        return m_BasePath;
    }
    
    /*
     * Is a directory
     */
    inline bool IsDir() const
    {
        return m_IsDir;
    }
    
    /*
     *
     */
    inline bool IsValid() const
    {
        return !m_AbsolutePath.empty();
    }

private:
    void Configure(const std::string& basePath, const std::string& fileName, bool isDir)
    {
        m_BasePath = basePath;
        m_Name = fileName;
        m_IsDir = isDir;
        
        if (!StringUtils::EndsWith(m_BasePath, "/")) {
            m_BasePath += "/";
        }
        
        if (isDir && !StringUtils::EndsWith(m_Name, "/")) {
            m_Name += "/";
        }
        
        if (StringUtils::StartsWith(m_Name, "/")) {
            m_Name = m_Name.substr(1, m_Name.length() - 1);
        }
        
        m_AbsolutePath = m_BasePath + m_Name;
        
        size_t dotsNum = std::count(m_Name.begin(), m_Name.end(), '.');
        bool isDotOrDotDot = (dotsNum == m_Name.length() && isDir);
        
        if (!isDotOrDotDot) {
            std::size_t found = m_Name.rfind(".");
            if (found != std::string::npos) {
                m_BaseName = m_Name.substr(0, found);
                if (found < m_Name.length()) {
                    m_Extension = m_Name.substr(found, m_Name.length() - found);
                }
            }
        }
    }
    
private:
    std::string m_Name;
    std::string m_BaseName;
    std::string m_Extension;
    std::string m_AbsolutePath;
    std::string m_BasePath;
    bool m_IsDir;
};
    
inline bool operator ==(const FileInfo& fi1, const FileInfo& fi2)
{
    return fi1.AbsolutePath() == fi2.AbsolutePath();
}
    
inline bool operator <(const FileInfo& fi1, const FileInfo& fi2)
{
    return fi1.AbsolutePath() < fi2.AbsolutePath();
}
    
}; // namespace vfspp

#endif // FILEINFO_HPP
