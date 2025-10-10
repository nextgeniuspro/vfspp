#ifndef VIRTUALFILESYSTEM_HPP
#define VIRTUALFILESYSTEM_HPP

#include "IFileSystem.h"
#include "IFile.h"
#include "Alias.hpp"
#include "ThreadingPolicy.hpp"


namespace vfspp
{

template <typename ThreadingPolicy>
class VirtualFileSystem;

template <typename ThreadingPolicy>
using VirtualFileSystemPtr = std::shared_ptr<VirtualFileSystem<ThreadingPolicy>>;

template <typename ThreadingPolicy>
using VirtualFileSystemWeakPtr = std::weak_ptr<VirtualFileSystem<ThreadingPolicy>>;
    

template <typename ThreadingPolicy>
class VirtualFileSystem final
{
public:
    typedef std::list<IFileSystemPtr> TFileSystemList;
    typedef std::unordered_map<Alias, TFileSystemList, Alias::Hash> TFileSystemMap;
    
public:
    VirtualFileSystem()
    {
    }

    ~VirtualFileSystem()
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
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
    void AddFileSystem(const Alias& alias, IFileSystemPtr filesystem)
    {
        if (!filesystem) {
            return;
        }

        auto lock = ThreadingPolicy::Lock(m_Mutex);

        m_FileSystems[alias].push_back(filesystem);
        if (std::find(m_SortedAlias.begin(), m_SortedAlias.end(), alias) == m_SortedAlias.end()) {
            m_SortedAlias.push_back(alias);
        }
        std::sort(m_SortedAlias.begin(), m_SortedAlias.end(), [](const Alias& first, const Alias& second) {
            return first.Length() > second.Length();
        });
    }

    void AddFileSystem(std::string alias, IFileSystemPtr filesystem)
    {
        AddFileSystem(Alias(std::move(alias)), filesystem);
    }

    template <template <typename> class FileSystemType, typename... Args>
    [[nodiscard]] auto CreateFileSystem(const Alias& alias, Args&&... args)
        -> std::optional<std::shared_ptr<FileSystemType<ThreadingPolicy>>>
    {
        using ConcreteFileSystem = FileSystemType<ThreadingPolicy>;
        static_assert(std::is_base_of_v<IFileSystem, ConcreteFileSystem>,
                      "FileSystemType must derive from IFileSystem");

        auto filesystem = std::make_shared<ConcreteFileSystem>(std::forward<Args>(args)...);
        if (!filesystem) {
            return {};
        }

        filesystem->Initialize();
        if (!filesystem->IsInitialized()) {
            return {};
        }

        AddFileSystem(alias, filesystem);
        return filesystem;
    }

    template <template <typename> class FileSystemType, typename... Args>
    [[nodiscard]] auto CreateFileSystem(std::string alias, Args&&... args)
        -> std::optional<std::shared_ptr<FileSystemType<ThreadingPolicy>>>
    {
        return CreateFileSystem<FileSystemType>(Alias(std::move(alias)), std::forward<Args>(args)...);
    }
    
    /*
     * Remove registered filesystem
     */
    void RemoveFileSystem(const Alias& alias, IFileSystemPtr filesystem)
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);

        auto it = m_FileSystems.find(alias);
        if (it != m_FileSystems.end()) {
            it->second.remove(filesystem);
            if (it->second.empty()) {
                m_FileSystems.erase(it);
                m_SortedAlias.erase(std::remove(m_SortedAlias.begin(), m_SortedAlias.end(), alias), m_SortedAlias.end());
            }
        }
    }

    void RemoveFileSystem(std::string alias, IFileSystemPtr filesystem)
    {
        RemoveFileSystem(Alias(std::move(alias)), filesystem);
    }

    /*
     * Check if filesystem with 'alias' added
     */
    bool HasFileSystem(const Alias& alias, IFileSystemPtr fileSystem) const
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        auto it = m_FileSystems.find(alias);
        if (it != m_FileSystems.end()) {
            return std::find(it->second.begin(), it->second.end(), fileSystem) != it->second.end();
        }
        return false;
    }

    bool HasFileSystem(std::string alias, IFileSystemPtr fileSystem) const
    {
        return HasFileSystem(Alias(std::move(alias)), fileSystem);
    }

    /*
     * Unregister all filesystems with 'alias'
     */
    void UnregisterAlias(const Alias& alias)
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        m_FileSystems.erase(alias);
        m_SortedAlias.erase(std::remove(m_SortedAlias.begin(), m_SortedAlias.end(), alias), m_SortedAlias.end());
    }

    void UnregisterAlias(std::string alias)
    {
        UnregisterAlias(Alias(std::move(alias)));
    }
    
    /*
     * Check if there any filesystem with 'alias' registered
     */
    bool IsAliasRegistered(const Alias& alias) const
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return m_FileSystems.find(alias) != m_FileSystems.end();
    }

    bool IsAliasRegistered(std::string alias) const
    {
        return IsAliasRegistered(Alias(std::move(alias)));
    }
    
    /*
     * Get all added filesystems with 'alias'
     */
    const TFileSystemList& GetFilesystems(const Alias& alias)
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return GetFilesystemsST(alias);
    }

    const TFileSystemList& GetFilesystems(std::string alias)
    {
        return GetFilesystems(Alias(std::move(alias)));
    }

private:
    template<typename Callback>
    auto VisitMountedFileSystems(const std::string& absolutePath, Callback&& callback) const
    {
        using CallbackResult = decltype(callback(std::declval<IFileSystemPtr>(), std::declval<const std::string&>(), std::declval<bool>()));

        for (const Alias& alias : m_SortedAlias) {
            const std::string& aliasString = alias.String();
            if (!absolutePath.starts_with(aliasString)) {
                continue;
            }

            std::string relativePath = absolutePath.substr(alias.Length());

            const TFileSystemList& filesystems = GetFilesystemsST(alias);
            if (filesystems.empty()) {
                continue;
            }

            for (auto it = filesystems.rbegin(); it != filesystems.rend(); ++it) {
                IFileSystemPtr fs = *it;
                bool isMain = (fs == filesystems.front());

                CallbackResult result = callback(fs, relativePath, isMain);
                if (result) {
                    return result;
                }
            }
        }

        return CallbackResult{};
    }

public:
    
    /*
     * Iterate over all registered filesystems and find first ocurrences of file
     */
    IFilePtr OpenFile(const FileInfo& filePath, IFile::FileMode mode)
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);

        const std::string absolutePath = filePath.AbsolutePath();

        auto result = VisitMountedFileSystems(absolutePath, [&](IFileSystemPtr fs, const std::string& relativePath, bool isMain) -> std::optional<IFilePtr> {
            FileInfo realPath(fs->BasePath(), relativePath, false);
            if (fs->IsFileExists(realPath) || isMain) {
                IFilePtr file = fs->OpenFile(realPath, mode);
                if (file) {
                    return file;
                }
            }
            return std::nullopt;
        });

        if (result) {
            return result.value();
        }

        return nullptr;
    }

    std::string AbsolutePath(std::string_view relativePath)
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);

        std::string strRelativePath(relativePath);

        auto result = VisitMountedFileSystems(strRelativePath, [&](IFileSystemPtr fs, const std::string& strippedRelativePath, bool isMain) -> std::optional<std::string> {
            FileInfo realPath(fs->BasePath(), strippedRelativePath, false);
            if (fs->IsFileExists(realPath) || isMain) {
                return realPath.AbsolutePath();
            }
            return std::nullopt;
        });

        if (result) {
            return result.value();
        }

        return std::string();
    }
    
    /*
     * Check if file exists in any registered filesystem
     */
    bool IsFileExists(std::string_view relativePath) const
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);

        std::string strRelativePath(relativePath);

        auto result = VisitMountedFileSystems(strRelativePath, [&](IFileSystemPtr fs, const std::string& strippedRelativePath, bool /*isMain*/) -> std::optional<bool> {
            FileInfo realPath(fs->BasePath(), strippedRelativePath, false);
            if (fs->IsFileExists(realPath)) {
                return true;
            }
            return std::nullopt;
        });

        return result.value_or(false);
    }

    /*
     * List all files from all registered filesystems
     * Returns a vector of all file paths with their aliases
     * Files from later registered filesystems override earlier ones
     */
//    std::vector<std::string> ListAllFiles() const
//    {
//        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
//            std::lock_guard<std::mutex> lock(m_Mutex);
//            return ListAllFilesST();
//        } else {
//            return ListAllFilesST();
//        }
//    }

private:
//    std::vector<std::string> ListAllFilesST() const
//    {
//        std::vector<std::string> allFiles;
//        std::unordered_set<std::string> seenFiles;
//
//        for (const Alias& alias : m_SortedAlias) {
//            const std::string& aliasString = alias.String();
//            const TFileSystemList& filesystems = GetFilesystemsST(alias);
//            if (filesystems.empty()) {
//                continue;
//            }
//
//            // Enumerate reverse to give priority to later registered filesystems
//            for (auto it = filesystems.rbegin(); it != filesystems.rend(); ++it) {
//                IFileSystemPtr fs = *it;
//                if (!fs || !fs->IsInitialized()) {
//                    continue;
//                }
//
//                const IFileSystem::TFileList& fileList = fs->FileList();
//
//                for (const auto& [relativePath, filePtr] : fileList) {
//                    if (!filePtr || filePtr->GetFileInfo().IsDir()) {
//                        continue;
//                    }
//
//                    std::string fullPath;
//                    fullPath.reserve(aliasString.size() + relativePath.size() + 1);
//                    fullPath.append(aliasString);
//                    if (!relativePath.empty()) {
//                        if (relativePath.front() == '/') {
//                            fullPath.append(relativePath.c_str() + 1); // skip leading slash
//                        } else {
//                            fullPath.append(relativePath);
//                        }
//                    }
//
//                    // Only add if not seen before (priority to later filesystems)
//                    if (seenFiles.emplace(fullPath).second) {
//                        allFiles.push_back(std::move(fullPath));
//                    }
//                }
//            }
//        }
//
//        std::sort(allFiles.begin(), allFiles.end());
//        return allFiles;
//    }

    inline const TFileSystemList& GetFilesystemsST(const Alias& alias) const
    {
        auto it = m_FileSystems.find(alias);
        if (it != m_FileSystems.end()) {
            return it->second;
        }
        
        static TFileSystemList empty;
        return empty;
    }

    
private:
    TFileSystemMap m_FileSystems;
    std::vector<Alias> m_SortedAlias;
    mutable std::mutex m_Mutex;
};

using MultiThreadedVirtualFileSystem = VirtualFileSystem<MultiThreadedPolicy>;
using MultiThreadedVirtualFileSystemPtr = VirtualFileSystemPtr<MultiThreadedPolicy>;
using MultiThreadedVirtualFileSystemWeakPtr = VirtualFileSystemWeakPtr<MultiThreadedPolicy>;

using SingleThreadedVirtualFileSystem = VirtualFileSystem<SingleThreadedPolicy>;
using SingleThreadedVirtualFileSystemPtr = VirtualFileSystemPtr<SingleThreadedPolicy>;
using SingleThreadedVirtualFileSystemWeakPtr = VirtualFileSystemWeakPtr<SingleThreadedPolicy>;

    
}; // namespace vfspp

#endif // VIRTUALFILESYSTEM_HPP
