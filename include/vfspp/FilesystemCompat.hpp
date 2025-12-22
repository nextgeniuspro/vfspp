#ifndef VFSPP_FILESYSTEM_COMPAT_HPP
#define VFSPP_FILESYSTEM_COMPAT_HPP

#include "Global.h"

#ifdef VFSPP_DISABLE_STD_FILESYSTEM

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <cerrno>
#include <cstdio>
#include <fstream>
#include <cstdint>

namespace vfspp
{
namespace fs_compat
{

enum class perms : unsigned
{
    none = static_cast<unsigned>(0),
    owner_write = static_cast<unsigned>(S_IWUSR)
};

inline perms operator&(perms lhs, perms rhs)
{
    return static_cast<perms>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs));
}

inline perms operator|(perms lhs, perms rhs)
{
    return static_cast<perms>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs));
}

class path
{
public:
    path() = default;
    path(const std::string& p)
        : m_Path(NormalizeSeparators(p))
    {
    }

    path(const char* p)
        : m_Path(NormalizeSeparators(p ? std::string(p) : std::string()))
    {
    }

    bool empty() const
    {
        return m_Path.empty();
    }

    std::string generic_string() const
    {
        return m_Path;
    }

    std::string string() const
    {
        return m_Path;
    }

    const std::string& native() const
    {
        return m_Path;
    }

    path filename() const
    {
        return path(ExtractFilename(m_Path));
    }

    bool has_extension() const
    {
        return !ExtractExtension(m_Path).empty();
    }

    path extension() const
    {
        return path(ExtractExtension(m_Path));
    }

    path stem() const
    {
        return path(ExtractStem(m_Path));
    }

    path operator/(const path& other) const
    {
        if (other.m_Path.empty()) {
            return *this;
        }
        if (m_Path.empty()) {
            return other;
        }

        std::string combined = m_Path;
        if (combined.back() != '/') {
            combined.push_back('/');
        }

        std::string tail = other.m_Path;
        if (!tail.empty() && tail.front() == '/') {
            tail.erase(tail.begin());
        }

        combined += tail;
        return path(combined);
    }

private:
    static std::string NormalizeSeparators(const std::string& in)
    {
        std::string out;
        out.reserve(in.size());
        for (char c : in) {
            out.push_back(c == '\\' ? '/' : c);
        }
        return out;
    }

    static std::string ExtractFilename(const std::string& in)
    {
        if (in.empty()) {
            return {};
        }

        const auto pos = in.find_last_of('/') ;
        if (pos == std::string::npos) {
            return in;
        }
        if (pos + 1u >= in.size()) {
            return {};
        }
        return in.substr(pos + 1u);
    }

    static std::string ExtractExtension(const std::string& in)
    {
        const auto filename = ExtractFilename(in);
        const auto dotPos = filename.find_last_of('.');
        if (dotPos == std::string::npos || dotPos == 0u) {
            return {};
        }
        return filename.substr(dotPos);
    }

    static std::string ExtractStem(const std::string& in)
    {
        const auto filename = ExtractFilename(in);
        const auto dotPos = filename.find_last_of('.');
        if (dotPos == std::string::npos || dotPos == 0u) {
            return filename;
        }
        return filename.substr(0u, dotPos);
    }

private:
    std::string m_Path;
};

struct file_status
{
    perms permissions() const
    {
        return m_Perms;
    }

    bool is_directory() const
    {
        return m_IsDirectory;
    }

    bool is_regular_file() const
    {
        return m_IsRegular;
    }

    perms m_Perms = perms::none;
    bool m_IsDirectory = false;
    bool m_IsRegular = false;
};

inline file_status status(const std::string& p)
{
    struct stat st;
    if (::stat(p.c_str(), &st) != 0) {
        return {};
    }

    file_status fs;
    fs.m_Perms = (st.st_mode & S_IWUSR) ? perms::owner_write : perms::none;
    fs.m_IsDirectory = S_ISDIR(st.st_mode);
    fs.m_IsRegular = S_ISREG(st.st_mode);
    return fs;
}

inline file_status status(const path& p)
{
    return status(p.generic_string());
}

// Forward declarations for path-based functions so string overloads can call them
inline uintmax_t file_size(const path& p, std::error_code& ec);
inline void rename(const path& from, const path& to, std::error_code& ec);
inline bool remove(const path& p);

enum class copy_options
{
    skip_existing,
    overwrite_existing
};

inline bool copy_file(const path& from, const path& to, copy_options option);

inline uintmax_t file_size(const std::string& p, std::error_code& ec)
{
    struct stat st;
    if (::stat(p.c_str(), &st) != 0) {
        ec = std::error_code(errno, std::generic_category());
        return 0;
    }

    ec.clear();
    return static_cast<uintmax_t>(st.st_size);
}

inline bool exists(const std::string& p)
{
    struct stat st;
    return ::stat(p.c_str(), &st) == 0;
}

inline bool exists(const path& p)
{
    return exists(p.generic_string());
}

inline bool is_directory(const file_status& status)
{
    return status.is_directory();
}

inline bool is_directory(const std::string& p)
{
    struct stat st;
    return (::stat(p.c_str(), &st) == 0) && S_ISDIR(st.st_mode);
}

inline bool is_directory(const path& p)
{
    return is_directory(p.generic_string());
}

inline bool is_regular_file(const std::string& p)
{
    struct stat st;
    return (::stat(p.c_str(), &st) == 0) && S_ISREG(st.st_mode);
}

inline bool is_regular_file(const path& p)
{
    return is_regular_file(p.generic_string());
}

inline bool is_regular_file(const file_status& status)
{
    return status.is_regular_file();
}

inline uintmax_t file_size(const path& p, std::error_code& ec)
{
    struct stat st;
    if (::stat(p.generic_string().c_str(), &st) != 0) {
        ec = std::error_code(errno, std::generic_category());
        return 0;
    }

    ec.clear();
    return static_cast<uintmax_t>(st.st_size);
}
inline bool copy_file(const path& from, const path& to, copy_options option)
{
    if (!is_regular_file(from)) {
        return false;
    }

    const bool dstExists = exists(to);
    if (dstExists && option == copy_options::skip_existing) {
        return false;
    }

    std::ifstream src(from.generic_string(), std::ios::binary);
    std::ofstream dst(to.generic_string(), std::ios::binary | std::ios::trunc);
    if (!src.is_open() || !dst.is_open()) {
        return false;
    }

    dst << src.rdbuf();
    return dst.good();
}

inline bool copy_file(const std::string& from, const std::string& to, copy_options option)
{
    return copy_file(path(from), path(to), option);
}

inline void rename(const path& from, const path& to, std::error_code& ec)
{
    if (::rename(from.generic_string().c_str(), to.generic_string().c_str()) != 0) {
        ec = std::error_code(errno, std::generic_category());
        return;
    }
    ec.clear();
}

inline void rename(const std::string& from, const std::string& to, std::error_code& ec)
{
    rename(path(from), path(to), ec);
}

inline bool remove(const path& p)
{
    return ::remove(p.generic_string().c_str()) == 0;
}

inline bool remove(const std::string& p)
{
    return remove(path(p));
}

class directory_entry
{
public:
    directory_entry() = default;

    explicit directory_entry(const fs_compat::path& p)
        : m_Path(p)
        , m_Status(fs_compat::status(p))
    {
    }

    const fs_compat::path& path() const
    {
        return m_Path;
    }

    file_status status() const
    {
        return m_Status;
    }

private:
    fs_compat::path m_Path;
    file_status m_Status;
};

class directory_iterator
{
public:
    directory_iterator() = default;

    explicit directory_iterator(const std::string& root)
    {
        LoadEntries(root);
    }

    explicit directory_iterator(const path& root)
    {
        LoadEntries(root.generic_string());
    }

    directory_iterator begin() const
    {
        return *this;
    }

    directory_iterator end() const
    {
        directory_iterator it;
        it.m_Entries = m_Entries;
        it.m_Index = m_Entries ? m_Entries->size() : 0u;
        return it;
    }

    directory_iterator& operator++()
    {
        if (m_Index < Size()) {
            ++m_Index;
        }
        return *this;
    }

    const directory_entry& operator*() const
    {
        return (*m_Entries)[m_Index];
    }

    const directory_entry* operator->() const
    {
        return &(*m_Entries)[m_Index];
    }

    bool operator!=(const directory_iterator& other) const
    {
        return m_Entries != other.m_Entries || m_Index != other.m_Index;
    }

private:
    void LoadEntries(const std::string& root)
    {
        m_Entries = std::make_shared<std::vector<directory_entry>>();

        DIR* dir = ::opendir(root.c_str());
        if (!dir) {
            return;
        }

        struct dirent* ent;
        while ((ent = ::readdir(dir)) != nullptr) {
            const std::string name = ent->d_name;
            if (name == "." || name == "..") {
                continue;
            }

            path entryPath = path(root) / path(name);
            m_Entries->emplace_back(directory_entry(entryPath));
        }
        ::closedir(dir);
    }

    size_t Size() const
    {
        return m_Entries ? m_Entries->size() : 0u;
    }

private:
    std::shared_ptr<std::vector<directory_entry>> m_Entries;
    size_t m_Index = 0u;
};

} // namespace fs_compat
} // namespace vfspp

#endif // VFSPP_DISABLE_STD_FILESYSTEM

#endif // VFSPP_FILESYSTEM_COMPAT_HPP
