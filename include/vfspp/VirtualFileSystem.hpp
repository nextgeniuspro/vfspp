#ifndef VIRTUALFILESYSTEM_HPP
#define VIRTUALFILESYSTEM_HPP

#include "IFileSystem.h"
#include "IFile.h"

namespace vfspp
{

using VirtualFileSystemPtr = std::shared_ptr<class VirtualFileSystem>;
using VirtualFileSystemWeakPtr = std::weak_ptr<class VirtualFileSystem>;
    

class VirtualFileSystem final
{
public:
    typedef std::list<IFileSystemPtr> TFileSystemList;
    typedef std::unordered_map<std::string, TFileSystemList> TFileSystemMap;
    
public:
    VirtualFileSystem()
    {
    }

    ~VirtualFileSystem()
    {
        for (const auto& fs : m_FileSystems) {
            for (const auto& f : fs.second) {
                f->Shutdown();
            }
        }
    }
    
    /*
     * Register new filesystem. Alias is a base prefix to file access.
     * For ex. registered filesystem has base path '/home/media', but registered
     * with alias '/', so it possible to access files with path '/filename'
     * instead of '/home/media/filename
     */
    void AddFileSystem(std::string alias, IFileSystemPtr filesystem)
    {
        if (!filesystem) {
            return;
        }

        if (!StringUtils::EndsWith(alias, "/")) {
            alias += "/";
        }
        
        std::function<void()> fn = [&]() {
            m_FileSystems[alias].push_back(filesystem);
            if (std::find(m_SortedAlias.begin(), m_SortedAlias.end(), alias) == m_SortedAlias.end()) {
                m_SortedAlias.push_back(alias);
            }
            std::sort(m_SortedAlias.begin(), m_SortedAlias.end(), [](const std::string& a1, const std::string& a2) {
                return a1.length() > a2.length();
            });
        };
        
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            fn();
        } else {
            fn();
        }
    }
    
    /*
     * Remove registered filesystem
     */
    void RemoveFileSystem(std::string alias, IFileSystemPtr filesystem)
    {
        if (!StringUtils::EndsWith(alias, "/")) {
            alias += "/";
        }

        std::function<void()> fn = [&]() {
            auto it = m_FileSystems.find(alias);
            if (it != m_FileSystems.end()) {
                it->second.remove(filesystem);
                if (it->second.empty()) {
                    m_FileSystems.erase(it);
                    m_SortedAlias.erase(std::remove(m_SortedAlias.begin(), m_SortedAlias.end(), alias), m_SortedAlias.end());
                }
            }
        };

        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            fn();
        } else {
            fn();
        }
    }

    /*
     * Check if filesystem with 'alias' added
     */
    bool HasFileSystem(std::string alias, IFileSystemPtr fileSystem) const
    {
        if (!StringUtils::EndsWith(alias, "/")) {
            alias += "/";
        }

        std::function<bool()> fn = [&]() -> bool {
            auto it = m_FileSystems.find(alias);
            if (it != m_FileSystems.end()) {
                return (std::find(it->second.begin(), it->second.end(), fileSystem) != it->second.end());
            }
            return false;
        };

        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return fn();
        } else {
            return fn();
        }
    }

    /*
     * Unregister all filesystems with 'alias'
     */
    void UnregisterAlias(std::string alias)
    {
        if (!StringUtils::EndsWith(alias, "/")) {
            alias += "/";
        }

        std::function<void()> fn = [&]() {
            m_FileSystems.erase(alias);
            m_SortedAlias.erase(std::remove(m_SortedAlias.begin(), m_SortedAlias.end(), alias), m_SortedAlias.end());
        };

        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            fn();
        } else {
            fn();
        }
    }
    
    /*
     * Check if there any filesystem with 'alias' registered
     */
    bool IsAliasRegistered(std::string alias) const
    {
        if (!StringUtils::EndsWith(alias, "/")) {
            alias += "/";
        }

        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return (m_FileSystems.find(alias) != m_FileSystems.end());
        } else {
            return (m_FileSystems.find(alias) != m_FileSystems.end());
        }
    }
    
    /*
     * Get all added filesystems with 'alias'
     */
    const TFileSystemList& GetFilesystems(std::string alias)
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return GetFilesystemsST(alias);
        } else {
            return GetFilesystemsST(alias);
        }
    }
    
    /*
     * Iterate over all registered filesystems and find first ocurrences of file
     */
    IFilePtr OpenFile(const FileInfo& filePath, IFile::FileMode mode)
    {
        std::function<IFilePtr()> fn = [&]() -> IFilePtr {
            for (const std::string& alias : m_SortedAlias) {
                if (!StringUtils::StartsWith(filePath.AbsolutePath(), alias)) {
                    continue;
                }

                // Strip alias from file path
                std::string relativePath = filePath.AbsolutePath().substr(alias.length());
                
                // Enumerate reverse to get filesystems in order of registration
                const TFileSystemList& filesystems = GetFilesystemsST(alias);
                if (filesystems.empty()) {
                    continue;
                }
                
                for (auto it = filesystems.rbegin(); it != filesystems.rend(); ++it) {
                    // Is it last filesystem
                    IFileSystemPtr fs = *it;
                    bool isMain = (fs == filesystems.front());

                    // If file exists in filesystem we try to open it. 
                    // In case file not exists and we are in first filesystem we try to create new file if mode allows it
                    FileInfo realPath(fs->BasePath(), relativePath, false);
                    if (fs->IsFileExists(realPath) || isMain) {
                        IFilePtr file = fs->OpenFile(realPath, mode);
                        if (file) {
                            return file;
                        }
                    }
                }
            }
            
            return nullptr;
        };

        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return fn();
        } else {
            return fn();
        }
    }

    std::string AbsolutePath(std::string_view relativePath)
    {
        std::function<std::string()> fn = [&]() -> std::string {
            std::string strRelativePath = std::string(relativePath);
            for (const std::string& alias : m_SortedAlias) {
                if (!StringUtils::StartsWith(strRelativePath, alias)) {
                    continue;
                }

                // Strip alias from file path
                std::string strippedRelativePath = strRelativePath.substr(alias.length());

                // Enumerate reverse to get filesystems in order of registration
                const TFileSystemList& filesystems = GetFilesystemsST(alias);
                if (filesystems.empty()) {
                    continue;
                }

                for (auto it = filesystems.rbegin(); it != filesystems.rend(); ++it) {
                    // Is it last filesystem
                    IFileSystemPtr fs = *it;
                    bool isMain = (fs == filesystems.front());

                    // If file exists in filesystem we try to open it. 
                    // In case file not exists and we are in first filesystem we try to create new file if mode allows it
                    FileInfo realPath(fs->BasePath(), strippedRelativePath, false);
                    if (fs->IsFileExists(realPath) || isMain) {
                        return realPath.AbsolutePath();
                    }
                }
            }

            return "";
            };

        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return fn();
        }
        else {
            return fn();
        }
    }

private:
    inline const TFileSystemList& GetFilesystemsST(std::string alias)
    {
        if (!StringUtils::EndsWith(alias, "/")) {
            alias += "/";
        }

        auto it = m_FileSystems.find(alias);
        if (it != m_FileSystems.end()) {
            return it->second;
        }
        
        static TFileSystemList empty;
        return empty;
    }

    
private:
    TFileSystemMap m_FileSystems;
    std::vector<std::string> m_SortedAlias;
    mutable std::mutex m_Mutex;
};
    
}; // namespace vfspp

#endif // VIRTUALFILESYSTEM_HPP
