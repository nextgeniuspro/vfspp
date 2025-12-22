#ifndef VFSPP_FILEINFO_HPP
#define VFSPP_FILEINFO_HPP

#include "Global.h"

#ifdef VFSPP_DISABLE_STD_FILESYSTEM
#include "FilesystemCompat.hpp"
namespace fs = vfspp::fs_compat;
#else
namespace fs = std::filesystem;
#endif

namespace vfspp
{
    
class FileInfo final
{
public:
    FileInfo(const std::string& aliasPath, const std::string& basePath, const std::string& fileName)
    {
        Configure(aliasPath, basePath, fileName);
    }

    FileInfo() = delete;
    ~FileInfo() = default;
    
    /*
     * Get file name with extension
     */
    inline const std::string& Filename() const
    {
        return m_Filename;
    }
    
    /*
     * Get file name without extension
     */
    inline const std::string& BaseFilename() const
    {
        return m_BaseFilename;
    }
    
    /*
     * Get file extension
     */
    inline const std::string& Extension() const
    {
        return m_Extension;
    }

    /*
    * Get path to the file without alias or base path
    */
    inline const std::string& FilePath() const
    {
        return m_Filepath;
    }
    
    /*
     * Get aliased file path, the path used to access file in virtual filesystem
     */
    inline const std::string& VirtualPath() const
    {
        return m_VirtualPath;
    }

    /*
     * Get native file path, the path used to access file in native filesystem
     */
    inline const std::string& NativePath() const
    {
        return m_NativePath;
    }

private:
    void Configure(const std::string& aliasPath, const std::string& basePath, const std::string& fileName)
    {
        // Remove alias path from file name if any
        std::string strippedFileName = fileName;
        if (!basePath.empty() && fileName.find(basePath) == 0) {
            strippedFileName = fileName.substr(basePath.length());
        }

        // Strip leading separators
        while (!strippedFileName.empty() && (strippedFileName.front() == '/' || strippedFileName.front() == '\\')) {
            strippedFileName.erase(strippedFileName.begin());
        }

        const auto filePath = fs::path(strippedFileName);

        m_Filepath = filePath.generic_string();
        m_VirtualPath = (fs::path(aliasPath) / filePath).generic_string();
        m_NativePath = (fs::path(basePath) / filePath).generic_string();

        m_Filename = filePath.filename().string();
        m_Extension = filePath.has_extension() ? filePath.extension().string() : "";
        m_BaseFilename = filePath.stem().string();
    }
    
private:
    std::string m_Filename;
    std::string m_BaseFilename;
    std::string m_Extension;
    
    std::string m_Filepath;
    std::string m_VirtualPath;
    std::string m_NativePath;
};
    
inline bool operator ==(const FileInfo& fi1, const FileInfo& fi2)
{
    return fi1.VirtualPath() == fi2.VirtualPath();
}
    
inline bool operator <(const FileInfo& fi1, const FileInfo& fi2)
{
    return fi1.VirtualPath() < fi2.VirtualPath();
}
    
}; // namespace vfspp

#endif // VFSPP_FILEINFO_HPP
