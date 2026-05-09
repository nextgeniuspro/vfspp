#ifndef VFSPP_MEMORYFILESYSTEM_HPP
#define VFSPP_MEMORYFILESYSTEM_HPP

#include "IFileSystem.h"
#include "Global.h"
#include "MemoryFile.hpp"

namespace vfspp
{

using MemoryFileSystemPtr = std::shared_ptr<class MemoryFileSystem>;
using MemoryFileSystemWeakPtr = std::weak_ptr<class MemoryFileSystem>;


class MemoryFileSystem final : public IFileSystem
{
public:
    MemoryFileSystem(const std::string& aliasPath)
        : m_AliasPath(aliasPath)
    {
    }

    ~MemoryFileSystem()
    {
        Shutdown();
    }
    
    /*
     * Initialize filesystem, call this method as soon as possible
     */
    [[nodiscard]]
    virtual bool Initialize() override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return InitializeImpl();
    }

    /*
     * Shutdown filesystem
     */
    virtual void Shutdown() override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        ShutdownImpl();
    }
    
    /*
     * Check if filesystem is initialized
     */
    [[nodiscard]]
    virtual bool IsInitialized() const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return IsInitializedImpl();
    }
    
    /*
     * Get base path
     */
    [[nodiscard]]
    virtual const std::string& BasePath() const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return BasePathImpl();
    }
    
    /*
     * Get mounted path
    */
    [[nodiscard]]
    virtual const std::string& VirtualPath() const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return AliasPathImpl();
    }

    /*
     * Retrieve all files in filesystem. Heavy operation, avoid calling this often
     */
    [[nodiscard]]
    virtual EntriesList GetEntriesList(bool excludeDirectories = true) const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return GetEntriesListImpl(excludeDirectories);
    }
    
    /*
     * Check is readonly filesystem
     */
    [[nodiscard]]
    virtual bool IsReadOnly() const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return IsReadOnlyImpl();
    }
    
    /*
     * Open existing file for reading, if not exists returns null for readonly filesystem. 
     * If file not exists and filesystem is writable then create new file
     */
    virtual IFilePtr OpenFile(const std::string& virtualPath, IFile::FileMode mode) override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return OpenFileImpl(virtualPath, mode);
    }

    /*
     * Close file
     */
    virtual void CloseFile(IFilePtr file) override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        CloseFileImpl(file);
    }
    
    /*
     * Create file on writeable filesystem. Returns IFilePtr object for created file
     */
    virtual IFilePtr CreateFile(const std::string& virtualPath) override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return OpenFileImpl(virtualPath, IFile::FileMode::ReadWrite | IFile::FileMode::Truncate);
    }
    
    /*
     * Remove existing file on writable filesystem
     */
    virtual bool RemoveFile(const std::string& virtualPath) override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return RemoveFileImpl(virtualPath);
    }
    
    /*
     * Copy existing file on writable filesystem
     */
    virtual bool CopyFile(const std::string& srcVirtualPath, const std::string& dstVirtualPath, bool overwrite = false) override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return CopyFileImpl(srcVirtualPath, dstVirtualPath, overwrite);
    }
    
    /*
     * Rename existing file on writable filesystem
     */
    virtual bool RenameFile(const std::string& srcVirtualPath, const std::string& dstVirtualPath) override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return RenameFileImpl(srcVirtualPath, dstVirtualPath);
    }

    virtual bool CreateDirectory(const std::string& virtualPath) override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return CreateDirectoryImpl(virtualPath);
    }

    virtual bool RemoveDirectory(const std::string& virtualPath, bool recursive = false) override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return RemoveDirectoryImpl(virtualPath, recursive);
    }

    virtual bool RenameDirectory(const std::string& srcVirtualPath, const std::string& dstVirtualPath) override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return RenameDirectoryImpl(srcVirtualPath, dstVirtualPath);
    }

    /*
     * Check if file exists on filesystem
     */
    [[nodiscard]]
    virtual bool IsFileExists(const std::string& virtualPath) const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return IsFileExistsImpl(virtualPath);
    }

    [[nodiscard]]
    virtual bool IsDirectoryExists(const std::string& virtualPath) const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return IsDirectoryExistsImpl(virtualPath);
    }

private:
    static std::string ParentPath(const std::string& path)
    {
        const size_t slash = path.find_last_of('/');
        if (slash == std::string::npos) {
            return {};
        }
        if (slash == 0) {
            return "/";
        }
        return path.substr(0, slash);
    }

    inline bool InitializeImpl()
    {
        if (m_IsInitialized) {
            return true;
        }
        m_IsInitialized = true;
        return m_IsInitialized;
    }

    inline void ShutdownImpl()
    {
        m_Entries.clear();
        
        m_IsInitialized = false;
    }

    inline bool IsInitializedImpl() const
    {
        return m_IsInitialized;
    }

    const std::string& BasePathImpl() const 
    {
        return m_AliasPath; // Native base path is always match to alias path for memory filesystem
    }

    const std::string& AliasPathImpl() const 
    {
        return m_AliasPath;
    }

    inline EntriesList GetEntriesListImpl(bool excludeDirectories) const
    {
        EntriesList list;
        list.reserve(m_Entries.size());
        for (const auto& [path, entry] : m_Entries) {
            if (excludeDirectories && entry.Info.IsDirectory()) {
                continue;
            }
            list.push_back(entry.Info);
        }
        return list;
    }

    inline bool IsReadOnlyImpl() const
    {
        return false;
    }

    inline IFilePtr OpenFileImpl(const std::string& virtualPath, IFile::FileMode mode)
    {
        EntryInfo fileInfo(AliasPathImpl(), BasePathImpl(), virtualPath);

        const auto existingIt = m_Entries.find(fileInfo.VirtualPath());
        if (existingIt != m_Entries.end() && existingIt->second.Info.IsDirectory()) {
            return nullptr;
        }

        InsertMissingDirectoryEntries(fileInfo.VirtualPath());
        auto entryResult = m_Entries.try_emplace(fileInfo.VirtualPath(), fileInfo, std::make_shared<MemoryFileObject>());
        auto entryIt = entryResult.first;

        auto& entry = entryIt->second;
        if (!entry.Object) {
            entry.Object = std::make_shared<MemoryFileObject>();
        }
        if (!entry.Object) {
            return nullptr;
        }

        auto file = std::make_shared<MemoryFile>(entry.Info, entry.Object);
        if (!file || !file->Open(mode)) {
            return nullptr;
        }

        entry.OpenedHandles.push_back(file);

        return file;
    }

    inline void CloseFileImpl(IFilePtr file)
    {
        CloseFileAndCleanupOpenedHandles(file);
    }

    inline bool RemoveFileImpl(const std::string& virtualPath)
    {
        EntryInfo fileInfo(AliasPathImpl(), BasePathImpl(), virtualPath);
        auto it = m_Entries.find(fileInfo.VirtualPath());
        if (it == m_Entries.end() || it->second.Info.IsDirectory()) {
            return false;
        }

        CloseOpenedHandlesForPrefix(fileInfo.VirtualPath());

        m_Entries.erase(it);

        return true;
    }

    inline bool CopyFileImpl(const std::string& srcVirtualPath, const std::string& dstVirtualPath, bool overwrite = false)
    {
        const EntryInfo srcInfo(AliasPathImpl(), BasePathImpl(), srcVirtualPath);
        const EntryInfo dstInfo(AliasPathImpl(), BasePathImpl(), dstVirtualPath);

        const auto srcIt = m_Entries.find(srcInfo.VirtualPath());
        if (srcIt == m_Entries.end() || srcIt->second.Info.IsDirectory()) {
            return false;
        }

        const auto destIt = m_Entries.find(dstInfo.VirtualPath());

        if (destIt != m_Entries.end() && (!overwrite || destIt->second.Info.IsDirectory())) {
            return false;
        }

        if (destIt != m_Entries.end()) {
            m_Entries.erase(destIt);
        }

        MemoryFileObjectPtr newObject;
        if (srcIt->second.Object) {
            newObject = std::make_shared<MemoryFileObject>(*srcIt->second.Object);
        } else {
            newObject = std::make_shared<MemoryFileObject>();
        }

        InsertMissingDirectoryEntries(dstInfo.VirtualPath());
        m_Entries.insert({dstInfo.VirtualPath(), Entry(dstInfo, std::move(newObject))});

        return true;
    }

    inline bool RenameFileImpl(const std::string& srcVirtualPath, const std::string& dstVirtualPath)
    {
        bool result = CopyFileImpl(srcVirtualPath, dstVirtualPath, false);
        if (result)  {
            result = RemoveFileImpl(srcVirtualPath);
        }
        
        return result;
    }

    inline bool CreateDirectoryImpl(const std::string& virtualPath)
    {
        const EntryInfo directoryInfo(AliasPathImpl(), BasePathImpl(), virtualPath, EntryType::Directory);
        if (directoryInfo.VirtualPath() == AliasPathImpl()) {
            return true;
        }

        const auto it = m_Entries.find(directoryInfo.VirtualPath());
        if (it != m_Entries.end()) {
            return it->second.Info.IsDirectory();
        }

        InsertMissingDirectoryEntries(directoryInfo.VirtualPath(), true);
        return true;
    }

    inline bool RemoveDirectoryImpl(const std::string& virtualPath, bool recursive)
    {
        const EntryInfo directoryInfo(AliasPathImpl(), BasePathImpl(), virtualPath, EntryType::Directory);
        auto it = m_Entries.find(directoryInfo.VirtualPath());
        if (it == m_Entries.end() || it->second.Info.IsFile()) {
            return false;
        }

        const auto descendants = CollectDescendantPaths(directoryInfo.VirtualPath());
        if (!recursive && !descendants.empty()) {
            return false;
        }

        CloseOpenedHandlesForPrefix(directoryInfo.VirtualPath());
        for (const auto& descendantPath : descendants) {
            m_Entries.erase(descendantPath);
        }
        m_Entries.erase(it);
        return true;
    }

    inline bool RenameDirectoryImpl(const std::string& srcVirtualPath, const std::string& dstVirtualPath)
    {
        const EntryInfo srcInfo(AliasPathImpl(), BasePathImpl(), srcVirtualPath, EntryType::Directory);
        const EntryInfo dstInfo(AliasPathImpl(), BasePathImpl(), dstVirtualPath, EntryType::Directory);

        const auto srcIt = m_Entries.find(srcInfo.VirtualPath());
        if (srcIt == m_Entries.end() || srcIt->second.Info.IsFile()) {
            return false;
        }

        if (m_Entries.find(dstInfo.VirtualPath()) != m_Entries.end()) {
            return false;
        }

        InsertMissingDirectoryEntries(dstInfo.VirtualPath());

        const auto affectedPaths = CollectAffectedPaths(srcInfo.VirtualPath());
        std::vector<std::pair<std::string, Entry>> rebuiltEntries;
        rebuiltEntries.reserve(affectedPaths.size());

        for (const auto& oldPath : affectedPaths) {
            auto entryIt = m_Entries.find(oldPath);
            if (entryIt == m_Entries.end()) {
                continue;
            }

            Entry rebuilt = std::move(entryIt->second);
            const std::string suffix = oldPath.substr(srcInfo.VirtualPath().size());
            const std::string newPath = dstInfo.VirtualPath() + suffix;
            rebuilt.Info = EntryInfo(AliasPathImpl(), BasePathImpl(), newPath, rebuilt.Info.Type());
            rebuiltEntries.emplace_back(newPath, std::move(rebuilt));
            m_Entries.erase(entryIt);
        }

        for (auto& [newPath, rebuiltEntry] : rebuiltEntries) {
            m_Entries.emplace(newPath, std::move(rebuiltEntry));
        }

        return true;
    }

    inline bool IsFileExistsImpl(const std::string& virtualPath) const
    {
        const EntryInfo fileInfo(AliasPathImpl(), BasePathImpl(), virtualPath);
        const auto it = m_Entries.find(fileInfo.VirtualPath());
        return it != m_Entries.end() && it->second.Info.IsFile();
    }

    inline bool IsDirectoryExistsImpl(const std::string& virtualPath) const
    {
        const EntryInfo directoryInfo(AliasPathImpl(), BasePathImpl(), virtualPath, EntryType::Directory);
        const auto it = m_Entries.find(directoryInfo.VirtualPath());
        return it != m_Entries.end() && it->second.Info.IsDirectory();
    }

    void InsertMissingDirectoryEntries(const std::string& virtualPath, bool includeSelf = false)
    {
        std::string currentPath = includeSelf ? virtualPath : ParentPath(virtualPath);
        std::vector<std::string> missingDirectories;

        while (!currentPath.empty()) {
            if (currentPath == AliasPathImpl()) {
                break;
            }

            const auto entryIt = m_Entries.find(currentPath);
            if (entryIt != m_Entries.end()) {
                break;
            }

            missingDirectories.push_back(currentPath);
            currentPath = ParentPath(currentPath);
        }

        for (auto it = missingDirectories.rbegin(); it != missingDirectories.rend(); ++it) {
            EntryInfo directoryInfo(AliasPathImpl(), BasePathImpl(), *it, EntryType::Directory);
            m_Entries.emplace(directoryInfo.VirtualPath(), Entry(directoryInfo, nullptr));
        }
    }

    std::vector<std::string> CollectDescendantPaths(const std::string& virtualPath) const
    {
        std::vector<std::string> descendants;
        const std::string prefix = virtualPath + "/";
        for (const auto& [entryPath, entry] : m_Entries) {
            if (entryPath.starts_with(prefix)) {
                descendants.push_back(entryPath);
            }
        }
        return descendants;
    }

    std::vector<std::string> CollectAffectedPaths(const std::string& virtualPath) const
    {
        std::vector<std::string> affectedPaths;
        affectedPaths.push_back(virtualPath);

        const auto descendants = CollectDescendantPaths(virtualPath);
        affectedPaths.insert(affectedPaths.end(), descendants.begin(), descendants.end());
        std::sort(affectedPaths.begin(), affectedPaths.end(), [](const std::string& lhs, const std::string& rhs) {
            return lhs.size() > rhs.size();
        });
        return affectedPaths;
    }

    void CloseOpenedHandlesForPrefix(const std::string& virtualPathPrefix)
    {
        const std::string childPrefix = virtualPathPrefix + "/";
        for (auto& [entryPath, entry] : m_Entries) {
            if (entryPath != virtualPathPrefix && !entryPath.starts_with(childPrefix)) {
                continue;
            }

            for (const auto& weakHandle : entry.OpenedHandles) {
                if (auto handle = weakHandle.lock()) {
                    handle->Close();
                }
            }
            entry.CleanupOpenedHandles();
        }
    }

    inline void CloseFileAndCleanupOpenedHandles(IFilePtr fileToClose = nullptr)
    {
        if (fileToClose) {
            const auto absolutePath = fileToClose->GetEntryInfo().VirtualPath();
            
            const auto it = m_Entries.find(absolutePath);
            if (it == m_Entries.end()) {
                return;
            }
            
            fileToClose->Close();
        }

        for (auto& [path, entry] : m_Entries) {
            entry.CleanupOpenedHandles(fileToClose);
        }
    }
    
private:
    std::string m_AliasPath;
    bool m_IsInitialized = false;
    mutable std::mutex m_Mutex;

    struct Entry
    {
        EntryInfo Info;
        MemoryFileObjectPtr Object;
        using WeakHandle = MemoryFileWeakPtr;
        std::vector<WeakHandle> OpenedHandles;

        Entry(const EntryInfo& info, MemoryFileObjectPtr object)
            : Info(info)
            , Object(object)
        {
        }

        void CleanupOpenedHandles(IFilePtr fileToExclude = nullptr)
        {
            OpenedHandles.erase(std::remove_if(OpenedHandles.begin(), OpenedHandles.end(), [&](const WeakHandle& weak) {
                return weak.expired() || weak.lock() == fileToExclude;
            }), OpenedHandles.end());
        }
    };

    std::unordered_map<std::string, Entry> m_Entries;
};

} // namespace vfspp

#endif // VFSPP_MEMORYFILESYSTEM_HPP
