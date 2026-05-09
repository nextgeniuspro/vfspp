#ifndef VFSPP_ENTRYINFO_HPP
#define VFSPP_ENTRYINFO_HPP

#include "Global.h"

#ifdef VFSPP_DISABLE_STD_FILESYSTEM
#include "FilesystemCompat.hpp"
namespace fs = vfspp::fs_compat;
#else
namespace fs = std::filesystem;
#endif

namespace vfspp
{

enum class EntryType
{
    File,
    Directory
};

class EntryInfo final
{
public:
    EntryInfo(const std::string& aliasPath, const std::string& basePath, const std::string& fileName, EntryType type = EntryType::File)
    {
        Configure(aliasPath, basePath, fileName, type);
    }

    EntryInfo() = delete;
    ~EntryInfo() = default;

    inline const std::string& Filename() const
    {
        return m_Filename;
    }

    inline const std::string& BaseFilename() const
    {
        return m_BaseFilename;
    }

    inline const std::string& Extension() const
    {
        return m_Extension;
    }

    inline const std::string& FilePath() const
    {
        return m_Filepath;
    }

    inline const std::string& VirtualPath() const
    {
        return m_VirtualPath;
    }

    inline const std::string& NativePath() const
    {
        return m_NativePath;
    }

    inline EntryType Type() const
    {
        return m_Type;
    }

    inline bool IsFile() const
    {
        return m_Type == EntryType::File;
    }

    inline bool IsDirectory() const
    {
        return m_Type == EntryType::Directory;
    }

private:
    static void StripTrailingSeparator(std::string& path)
    {
        while (path.size() > 1 && (path.back() == '/' || path.back() == '\\')) {
            path.pop_back();
        }
    }

    void Configure(const std::string& aliasPath, const std::string& basePath, const std::string& fileName, EntryType type)
    {
        m_Type = type;

        std::string strippedFileName = fileName;
        if (!aliasPath.empty() && fileName.find(aliasPath) == 0) {
            strippedFileName = fileName.substr(aliasPath.length());
        } else if (!basePath.empty() && fileName.find(basePath) == 0) {
            strippedFileName = fileName.substr(basePath.length());
        }

        while (!strippedFileName.empty() && (strippedFileName.front() == '/' || strippedFileName.front() == '\\')) {
            strippedFileName.erase(strippedFileName.begin());
        }

        const auto filePath = fs::path(strippedFileName);

        m_Filepath = filePath.generic_string();
        m_VirtualPath = (fs::path(aliasPath) / filePath).generic_string();
        m_NativePath = (fs::path(basePath) / filePath).generic_string();

        if (m_Type == EntryType::Directory) {
            StripTrailingSeparator(m_Filepath);
            StripTrailingSeparator(m_VirtualPath);
            StripTrailingSeparator(m_NativePath);
        }

        const auto normalizedPath = fs::path(m_Filepath);
        m_Filename = normalizedPath.filename().string();
        m_Extension = normalizedPath.has_extension() ? normalizedPath.extension().string() : "";
        m_BaseFilename = normalizedPath.stem().string();

        if (m_Type == EntryType::Directory) {
            m_Extension.clear();
            m_BaseFilename = m_Filename;
        }
    }

private:
    EntryType m_Type = EntryType::File;
    std::string m_Filename;
    std::string m_BaseFilename;
    std::string m_Extension;

    std::string m_Filepath;
    std::string m_VirtualPath;
    std::string m_NativePath;
};

inline bool operator ==(const EntryInfo& first, const EntryInfo& second)
{
    return first.VirtualPath() == second.VirtualPath();
}

inline bool operator <(const EntryInfo& first, const EntryInfo& second)
{
    return first.VirtualPath() < second.VirtualPath();
}

} // namespace vfspp

#endif // VFSPP_ENTRYINFO_HPP