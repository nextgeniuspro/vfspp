#ifndef VFSPP_VIRTUALFILESYSTEM_HPP
#define VFSPP_VIRTUALFILESYSTEM_HPP

#include "IFileSystem.h"
#include "IFile.h"
#include "Alias.hpp"
#include "ThreadingPolicy.hpp"

#include <concepts>
#include <type_traits>
#include <algorithm>


namespace vfspp
{

using VirtualFileSystemPtr = std::shared_ptr<class VirtualFileSystem>;
using VirtualFileSystemWeakPtr = std::weak_ptr<class VirtualFileSystem>;
    

class VirtualFileSystem final
{
public:
    using FileSystemList = std::vector<IFileSystemPtr>;
    using FileSystemMap = std::unordered_map<Alias, FileSystemList, Alias::Hash>;
    
public:
    VirtualFileSystem()
    {
    }

    ~VirtualFileSystem()
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
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

        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);

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

    template <typename FileSystemType, typename... Args>
    [[nodiscard]] auto CreateFileSystem(const Alias& alias, Args&&... args) -> std::optional<std::shared_ptr<FileSystemType>>
    {
        auto filesystem = std::make_shared<FileSystemType>(alias.String(), std::forward<Args>(args)...);
        if (!filesystem) {
            return {};
        }

        if (!filesystem->Initialize()) {
            return {};
        }

        AddFileSystem(alias, filesystem);
        return filesystem;
    }

    template <typename FileSystemType, typename... Args>
    [[nodiscard]] auto CreateFileSystem(std::string alias, Args&&... args) -> std::optional<std::shared_ptr<FileSystemType>>
    {
        return CreateFileSystem<FileSystemType>(Alias(std::move(alias)), std::forward<Args>(args)...);
    }
    
    /*
     * Remove registered filesystem
     */
    void RemoveFileSystem(const Alias& alias, IFileSystemPtr filesystem)
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);

        auto it = m_FileSystems.find(alias);
        if (it != m_FileSystems.end()) {
            auto &list = it->second;
            list.erase(std::remove(list.begin(), list.end(), filesystem), list.end());
            if (list.empty()) {
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
    [[nodiscard]]
    bool HasFileSystem(const Alias& alias, IFileSystemPtr fileSystem) const
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        auto it = m_FileSystems.find(alias);
        if (it != m_FileSystems.end()) {
            return std::find(it->second.begin(), it->second.end(), fileSystem) != it->second.end();
        }
        return false;
    }

    [[nodiscard]]
    bool HasFileSystem(std::string alias, IFileSystemPtr fileSystem) const
    {
        return HasFileSystem(Alias(std::move(alias)), fileSystem);
    }

    /*
     * Unregister all filesystems with 'alias'
     */
    void UnregisterAlias(const Alias& alias)
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
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
    [[nodiscard]]
    bool IsAliasRegistered(const Alias& alias) const
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return m_FileSystems.find(alias) != m_FileSystems.end();
    }

    [[nodiscard]]
    bool IsAliasRegistered(std::string alias) const
    {
        return IsAliasRegistered(Alias(std::move(alias)));
    }
    
    /*
     * Get all added filesystems with 'alias'
     */
    [[nodiscard]]
    std::optional<std::reference_wrapper<const FileSystemList>> GetFilesystems(const Alias& alias)
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        
        return GetFilesystemsImpl(alias);
    }

    [[nodiscard]]
    std::optional<std::reference_wrapper<const FileSystemList>> GetFilesystems(std::string alias)
    {
        return GetFilesystems(Alias(std::move(alias)));
    }

private:
    template<typename Callback>
    auto VisitMountedFileSystems(const std::string& virtualPath, Callback&& callback) const
    {
        using CallbackResult = decltype(callback(std::declval<IFileSystemPtr>(), std::declval<bool>()));

        for (const Alias& alias : m_SortedAlias) {
            if (!virtualPath.starts_with(alias.String())) {
                continue;
            }

            auto fsResult = GetFilesystemsImpl(alias);
            if (!fsResult) {
                continue;
            }

            const auto& filesystems = fsResult->get();
            for (auto it = filesystems.rbegin(); it != filesystems.rend(); ++it) {
                IFileSystemPtr fs = *it;
                bool isMain = (fs == filesystems.front());

                CallbackResult result = callback(fs, isMain);
                if (result) {
                    return result;
                }
            }
        }

        return CallbackResult{};
    }

public:
    
    /*
     * Iterate over all registered filesystems and find first ocurrences of file.
     * Iteration occurs from the most recently added filesystem to the oldest one.
     */
    IFilePtr OpenFile(const std::string& virtualPath, IFile::FileMode mode)
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);

        auto result = VisitMountedFileSystems(virtualPath, [&](IFileSystemPtr fs, bool /*isMain*/) -> std::optional<IFilePtr> {
            if (fs->IsFileExists(virtualPath)) {
                if (IFilePtr file = fs->OpenFile(virtualPath, mode)) {
                    return file;
                }
            }
            return std::nullopt;
        });

        return result.value_or(nullptr);
    }

    /*
     * Check if file exists in any registered filesystem
     */
    bool IsFileExists(const std::string& virtualPath) const
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);

        auto result = VisitMountedFileSystems(virtualPath, [&](IFileSystemPtr fs, bool /*isMain*/) -> std::optional<bool> {
            if (fs->IsFileExists(virtualPath)) {
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
    std::vector<std::string> ListAllFiles() const
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        
        std::vector<std::string> allFiles;
        std::unordered_set<std::string> seenFiles;

        for (const Alias& alias : m_SortedAlias) {
            auto fsResult = GetFilesystemsImpl(alias);
            if (!fsResult) {
                continue;
            }

            const auto& filesystems = fsResult->get();
            for (auto it = filesystems.rbegin(); it != filesystems.rend(); ++it) {

                IFileSystemPtr fs = *it;
                const IFileSystem::FilesList& fileList = fs->GetFilesList();

                for (const auto& fileInfo : fileList) {
                    const auto& virtualPath = fileInfo.VirtualPath();
                    if (seenFiles.emplace(virtualPath).second) {
                        allFiles.push_back(std::move(virtualPath));
                    }
                }
            }
        }   

        std::sort(allFiles.begin(), allFiles.end());
        return allFiles;
    }

private:
    [[nodiscard]]
    std::optional<std::reference_wrapper<const FileSystemList>> GetFilesystemsImpl(const Alias& alias) const
    {
        auto it = m_FileSystems.find(alias);
        if (it != m_FileSystems.end()) {
            return std::cref(it->second);
        }
        
        return {};
    }
    
private:
    FileSystemMap m_FileSystems;
    std::vector<Alias> m_SortedAlias;
    mutable std::mutex m_Mutex;
};

    
}; // namespace vfspp

#endif // VFSPP_VIRTUALFILESYSTEM_HPP
