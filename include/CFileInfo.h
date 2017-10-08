//
//  CFileInfo.h
//  vfspp
//
//  Created by Yevgeniy Logachev on 6/30/16.
//
//

#ifndef CFILEINFO_H
#define CFILEINFO_H

#include "VFS.h"

namespace vfspp
{
    
class CFileInfo final
{
public:
    CFileInfo();
    ~CFileInfo();
    
    CFileInfo(const std::string& filePath);
    CFileInfo(const std::string& basePath, const std::string& fileName, bool isDir);
    
    /*
     *
     */
    void Initialize(const std::string& basePath, const std::string& fileName, bool isDir);
    
    /*
     * Get file name
     */
    const std::string& Name() const;
    
    /*
     * Get file name without extension
     */
    const std::string& BaseName() const;
    
    /*
     * Get file extension
     */
    const std::string& Extension() const;
    
    /*
     * Get absolute file path
     */
    const std::string& AbsolutePath() const;
    
    /*
     * Directory where file locates
     */
    const std::string& BasePath() const;
    
    /*
     * Is a directory
     */
    bool IsDir() const;
    
    /*
     *
     */
    bool IsValid() const;
    
private:
    std::string m_Name;
    std::string m_BaseName;
    std::string m_Extension;
    std::string m_AbsolutePath;
    std::string m_BasePath;
    bool m_IsDir;
};
    
inline bool operator ==(const CFileInfo& fi1, const CFileInfo& fi2)
{
    return fi1.AbsolutePath() == fi2.AbsolutePath();
}
    
inline bool operator <(const CFileInfo& fi1, const CFileInfo& fi2)
{
    return fi1.AbsolutePath() < fi2.AbsolutePath();
}
    
}; // namespace vfspp

#endif /* CFILEINFO_H */
