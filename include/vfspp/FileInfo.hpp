#ifndef FILEINFO_HPP
#define FILEINFO_HPP

#include "Global.h"
#include "StringUtils.hpp"

namespace fs = std::filesystem;

namespace vfspp
{
    
class FileInfo final
{
public:
    FileInfo(const std::string& filePath)
    {
        Configure("", filePath, false);
    }

    FileInfo(const std::string& basePath, const std::string& fileName, bool isDir)
    {
        Configure(basePath, fileName, isDir);
    }

    FileInfo(const fs::path& path, bool isDir)
        : m_Path(path)
        , m_IsDir(isDir)
    {
    }

    FileInfo() = delete;

    ~FileInfo()
    {

    }
    
    /*
     * Get file name with extension
     */
    inline std::string Name() const
    {
        return m_Path.filename().string();
    }
    
    /*
     * Get file name without extension
     */
    inline std::string BaseName() const
    {
        return m_Path.stem().string();
    }
    
    /*
     * Get file extension
     */
    inline std::string Extension() const
    {
        return m_Path.extension().string();
    }
    
    /*
     * Get absolute file path
     */
    inline std::string AbsolutePath() const
    {
        return m_Path.string();
    }
    
    /*
     * Is a directory
     */
    inline bool IsDir() const
    {
        return m_IsDir;
    }

    inline const fs::path& Path() const
    {
        return m_Path;
    }
    
    /*
     *
     */
    inline bool IsValid() const
    {
        return !m_Path.string().empty();
    }

private:
    void Configure(const std::string& basePath, const std::string& fileName, bool isDir)
    {
        m_Path = (fs::path(basePath) / fs::path(fileName)).generic_string();
        m_IsDir = isDir;
    }
    
private:
    fs::path m_Path;
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
