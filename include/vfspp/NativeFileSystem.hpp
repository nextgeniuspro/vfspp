#ifndef VFSPP_NATIVEFILESYSTEM_HPP
#define VFSPP_NATIVEFILESYSTEM_HPP

#include "IFileSystem.h"
#include "Global.h"
#include "ThreadingPolicy.hpp"
#include "NativeFile.hpp"

#ifdef VFSPP_DISABLE_STD_FILESYSTEM
#include <sys/stat.h>
#include <dirent.h>
#endif

#ifdef VFSPP_DISABLE_STD_FILESYSTEM
#include "FilesystemCompat.hpp"
namespace fs = vfspp::fs_compat;
#else
namespace fs = std::filesystem;
#endif

namespace vfspp
{

using NativeFileSystemPtr = std::shared_ptr<class NativeFileSystem>;
using NativeFileSystemWeakPtr = std::weak_ptr<class NativeFileSystem>;

class NativeFileSystem final : public IFileSystem
{
public:
    NativeFileSystem(const std::string& aliasPath, const std::string& basePath, bool readOnly = false)
        : m_AliasPath(aliasPath)
        , m_BasePath(basePath)
        , m_ForceReadOnly(readOnly)
    {
    }

    ~NativeFileSystem()
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
     * Create file on writeable filesystem. Returns true if file created successfully
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

    virtual bool MakeDirectory(const std::string& virtualPath) override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return MakeDirectoryImpl(virtualPath);
    }

    virtual bool DeleteDirectory(const std::string& virtualPath, bool recursive = false) override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return DeleteDirectoryImpl(virtualPath, recursive);
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

    [[nodiscard]]
    virtual std::optional<EntryInfo> GetEntryInfo(const std::string& virtualPath) const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return GetEntryInfoImpl(virtualPath);
    }

private:
    struct Entry
    {
        EntryInfo Info;
        std::vector<NativeFileWeakPtr> OpenedHandles;

        explicit Entry(const EntryInfo& info)
            : Info(info)
        {
        }

        void CleanupOpenedHandles(IFilePtr fileToExclude = nullptr)
        {
            OpenedHandles.erase(std::remove_if(OpenedHandles.begin(), OpenedHandles.end(), [&](const NativeFileWeakPtr& weak) {
                return weak.expired() || weak.lock() == fileToExclude;
            }), OpenedHandles.end());
        }
    };

    inline bool InitializeImpl()
    {
        if (m_IsInitialized) {
            return true;
        }

        if (!IsDirectoryAccessible(BasePathImpl())) { // TODO: create if not exists flag
            return false;
        }

        BuildFilelist(AliasPathImpl(), BasePathImpl(), m_Entries);
        m_IsInitialized = true;
        return true;
    }

    inline void ShutdownImpl()
    {
        m_BasePath = "";
        m_AliasPath = "";
        m_Entries.clear();

        m_IsInitialized = false;
    }
    
    inline bool IsInitializedImpl() const
    {
        return m_IsInitialized;
    }

    const std::string& BasePathImpl() const
    {
        return m_BasePath;
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
        if (!IsInitializedImpl()) {
            return true;
        }

        if (m_ForceReadOnly) {
            return true;
        }

        auto perms = fs::status(BasePathImpl()).permissions();
        return (perms & fs::perms::owner_write) == fs::perms::none;
    }
    
    inline IFilePtr OpenFileImpl(const std::string& virtualPath, IFile::FileMode mode)
    {
        const bool requestWrite = IFile::ModeHasFlag(mode, IFile::FileMode::Write);
        if (IsReadOnlyImpl() && requestWrite) {
            return nullptr;
        }

        const EntryInfo fileInfo(AliasPathImpl(), BasePathImpl(), virtualPath);
        const auto it = m_Entries.find(fileInfo.VirtualPath());
        if (it != m_Entries.end() && it->second.Info.IsDirectory()) {
            return nullptr;
        }

        if (it == m_Entries.end() && requestWrite) {
            if (!EnsureDirectoryExists(ParentPath(fileInfo.NativePath()))) {
                return nullptr;
            }
            InsertMissingDirectoryEntries(fileInfo.VirtualPath());
            m_Entries.emplace(fileInfo.VirtualPath(), Entry(fileInfo));
        }

        const auto entryIt = m_Entries.find(fileInfo.VirtualPath());
        if (entryIt == m_Entries.end()) {
            return nullptr;
        }
        auto& entry = entryIt->second;

        if (entry.Info.IsDirectory()) {
            return nullptr;
        }

        NativeFilePtr file = std::make_shared<NativeFile>(entry.Info);
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
        if (IsReadOnlyImpl()) {
            return false;
        }

        const EntryInfo fileInfo(AliasPathImpl(), BasePathImpl(), virtualPath);
        auto it = m_Entries.find(fileInfo.VirtualPath());
        if (it == m_Entries.end() || it->second.Info.IsDirectory()) {
            return false;
        }

        CloseOpenedHandlesForPrefix(fileInfo.VirtualPath());

        const std::string nativePath = it->second.Info.NativePath();
        m_Entries.erase(it);

        return fs::remove(nativePath);
    }

    inline bool CopyFileImpl(const std::string& srcVirtualPath, const std::string& dstVirtualPath, bool overwrite = false)
    {
        if (IsReadOnlyImpl()) {
            return false;
        }

        // Check is src and dst start with alias path
        if (!IsPathOnAlias(srcVirtualPath) || !IsPathOnAlias(dstVirtualPath)) {
            return false;
        }

        const EntryInfo srcPath(AliasPathImpl(), BasePathImpl(), srcVirtualPath);
        const EntryInfo dstPath(AliasPathImpl(), BasePathImpl(), dstVirtualPath);

        const auto srcIt = m_Entries.find(srcPath.VirtualPath());
        if (srcIt == m_Entries.end() || srcIt->second.Info.IsDirectory()) {
            return false;
        }

        const auto dstIt = m_Entries.find(dstPath.VirtualPath());
        if (dstIt != m_Entries.end() && !overwrite) {
            return false;
        }

        if (!EnsureDirectoryExists(ParentPath(dstPath.NativePath()))) {
            return false;
        }

        auto option = overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::skip_existing;       
        const bool ok = fs::copy_file(srcPath.NativePath(), dstPath.NativePath(), option);
        if (!ok) {
            return false;
        }

        InsertMissingDirectoryEntries(dstPath.VirtualPath());

        const auto destIt = m_Entries.find(dstPath.VirtualPath());
        if (destIt != m_Entries.end()) {
            m_Entries.erase(destIt);
        }

        m_Entries.insert({dstPath.VirtualPath(), Entry(dstPath)});
        return true;
    }
    
    bool RenameFileImpl(const std::string& srcVirtualPath, const std::string& dstVirtualPath)
    {
        if (IsReadOnlyImpl()) {
            return false;
        }
        
        // Check is src and dst start with alias path
        if (!IsPathOnAlias(srcVirtualPath) || !IsPathOnAlias(dstVirtualPath)) {
            return false;
        }

        const EntryInfo srcPath(AliasPathImpl(), BasePathImpl(), srcVirtualPath);
        const EntryInfo dstPath(AliasPathImpl(), BasePathImpl(), dstVirtualPath);

        const auto srcIt = m_Entries.find(srcPath.VirtualPath());
        if (srcIt == m_Entries.end() || srcIt->second.Info.IsDirectory()) {
            return false;
        }

        const auto dstIt = m_Entries.find(dstPath.VirtualPath());
        if (dstIt != m_Entries.end()) {
            return false;
        }

        if (!EnsureDirectoryExists(ParentPath(dstPath.NativePath()))) {
            return false;
        }
        
        std::error_code ec;
        fs::rename(srcPath.NativePath(), dstPath.NativePath(), ec);
        if (ec) {
            return false;
        }

        InsertMissingDirectoryEntries(dstPath.VirtualPath());
        m_Entries.erase(srcIt);
        m_Entries.insert({dstPath.VirtualPath(), Entry(dstPath)});
        return true;
    }

    inline bool MakeDirectoryImpl(const std::string& virtualPath)
    {
        if (IsReadOnlyImpl()) {
            return false;
        }

        if (!IsPathOnAlias(virtualPath)) {
            return false;
        }

        const EntryInfo directoryInfo(AliasPathImpl(), BasePathImpl(), virtualPath, EntryType::Directory);
        if (directoryInfo.VirtualPath() == AliasPathImpl()) {
            return true;
        }

        const auto existingIt = m_Entries.find(directoryInfo.VirtualPath());
        if (existingIt != m_Entries.end()) {
            return existingIt->second.Info.IsDirectory();
        }

        if (!EnsureDirectoryExists(directoryInfo.NativePath())) {
            return false;
        }

        if (!fs::is_directory(directoryInfo.NativePath())) {
            return false;
        }

        InsertMissingDirectoryEntries(directoryInfo.VirtualPath(), true);
        return true;
    }

    inline bool DeleteDirectoryImpl(const std::string& virtualPath, bool recursive)
    {
        if (IsReadOnlyImpl()) {
            return false;
        }

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

        const bool removed = recursive
            ? RemoveDirectoryRecursive(directoryInfo.NativePath())
            : fs::remove(directoryInfo.NativePath());
        if (!removed) {
            return false;
        }

        for (const auto& descendantPath : descendants) {
            m_Entries.erase(descendantPath);
        }
        m_Entries.erase(it);
        return true;
    }

    inline bool RenameDirectoryImpl(const std::string& srcVirtualPath, const std::string& dstVirtualPath)
    {
        if (IsReadOnlyImpl()) {
            return false;
        }

        if (!IsPathOnAlias(srcVirtualPath) || !IsPathOnAlias(dstVirtualPath)) {
            return false;
        }

        const EntryInfo srcInfo(AliasPathImpl(), BasePathImpl(), srcVirtualPath, EntryType::Directory);
        const EntryInfo dstInfo(AliasPathImpl(), BasePathImpl(), dstVirtualPath, EntryType::Directory);

        const auto srcIt = m_Entries.find(srcInfo.VirtualPath());
        if (srcIt == m_Entries.end() || srcIt->second.Info.IsFile()) {
            return false;
        }

        if (m_Entries.find(dstInfo.VirtualPath()) != m_Entries.end()) {
            return false;
        }

        if (!EnsureDirectoryExists(ParentPath(dstInfo.NativePath()))) {
            return false;
        }

        std::error_code renameError;
        fs::rename(srcInfo.NativePath(), dstInfo.NativePath(), renameError);
        if (renameError) {
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
        const auto fileIt = m_Entries.find(fileInfo.VirtualPath());
        return fileIt != m_Entries.end() && fileIt->second.Info.IsFile() && fs::is_regular_file(fileIt->second.Info.NativePath());
    }

    inline bool IsDirectoryExistsImpl(const std::string& virtualPath) const
    {
        const EntryInfo directoryInfo(AliasPathImpl(), BasePathImpl(), virtualPath, EntryType::Directory);
        const auto it = m_Entries.find(directoryInfo.VirtualPath());
        return it != m_Entries.end() && it->second.Info.IsDirectory() && fs::is_directory(it->second.Info.NativePath());
    }

    inline std::optional<EntryInfo> GetEntryInfoImpl(const std::string& virtualPath) const
    {
        const EntryInfo fileInfo(AliasPathImpl(), BasePathImpl(), virtualPath);
        const auto fileIt = m_Entries.find(fileInfo.VirtualPath());
        if (fileIt == m_Entries.end()) {
            return std::nullopt;
        }

        const EntryInfo& entryInfo = fileIt->second.Info;
        if (entryInfo.IsFile() && fs::is_regular_file(entryInfo.NativePath())) {
            return entryInfo;
        }

        if (entryInfo.IsDirectory() && fs::is_directory(entryInfo.NativePath())) {
            return entryInfo;
        }

        return std::nullopt;
    }

    static bool IsDirectoryAccessible(const std::string& path)
    {
        const bool exists = fs::exists(path);
        const bool isDir = exists && fs::is_directory(path);
        if (exists && isDir) {
            return true;
        }

#ifdef VFSPP_DISABLE_STD_FILESYSTEM
        // Support for KallistiOS
        if (DIR* dir = ::opendir(path.c_str())) {
            ::closedir(dir);
            return true;
        }
#endif
        return false;
    }

    bool IsPathOnAlias(const std::string& virtualPath) const
    {
        if (virtualPath == AliasPathImpl()) {
            return true;
        }

        if (!virtualPath.starts_with(AliasPathImpl())) {
            return false;
        }

        if (AliasPathImpl() == "/") {
            return true;
        }

        return virtualPath.size() > AliasPathImpl().size();
    }

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

    bool EnsureDirectoryExists(const std::string& nativePath) const
    {
        if (nativePath.empty() || fs::is_directory(nativePath)) {
            return true;
        }

        const std::string parentPath = ParentPath(nativePath);
        if (!parentPath.empty() && parentPath != nativePath && !EnsureDirectoryExists(parentPath)) {
            return false;
        }

#ifdef VFSPP_DISABLE_STD_FILESYSTEM
        return ::mkdir(nativePath.c_str(), 0755) == 0 || errno == EEXIST;
#else
        std::error_code ec;
        const bool created = fs::create_directory(nativePath, ec);
        return (!ec && (created || fs::is_directory(nativePath)));
#endif
    }

    bool RemoveDirectoryRecursive(const std::string& nativePath) const
    {
        if (!fs::is_directory(nativePath)) {
            return false;
        }

        for (const auto& entry : fs::directory_iterator(nativePath)) {
            const std::string childPath = entry.path().generic_string();
            if (fs::is_directory(entry.status())) {
                if (!RemoveDirectoryRecursive(childPath)) {
                    return false;
                }
            } else if (!fs::remove(childPath)) {
                return false;
            }
        }

        return fs::remove(nativePath);
    }

    void InsertMissingDirectoryEntries(const std::string& virtualPath, bool includeSelf = false)
    {
        std::string currentPath = includeSelf ? virtualPath : ParentPath(virtualPath);
        std::vector<std::string> missingDirectories;

        while (!currentPath.empty() && currentPath != "/") {
            if (currentPath == AliasPathImpl() || currentPath + "/" == AliasPathImpl()) {
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
            m_Entries.emplace(directoryInfo.VirtualPath(), Entry(directoryInfo));
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

    void BuildFilelist(const std::string& aliasPath, const std::string& basePath, std::unordered_map<std::string, Entry>& outEntries)
    {
        for (const auto& entry : fs::directory_iterator(basePath)) {
            const bool isDirectory = fs::is_directory(entry.status());
            EntryInfo entryInfo(aliasPath, BasePathImpl(), entry.path().string(), isDirectory ? EntryType::Directory : EntryType::File);
            if (entryInfo.VirtualPath() != AliasPathImpl()) {
                outEntries.emplace(entryInfo.VirtualPath(), Entry(entryInfo));
            }

            if (isDirectory) {
                BuildFilelist(aliasPath, entry.path().string(), outEntries);
            }
        }
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
            const auto virtualPath = fileToClose->GetEntryInfo().VirtualPath();

            const auto it = m_Entries.find(virtualPath);
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
    mutable std::mutex m_Mutex;
    std::unordered_map<std::string, Entry> m_Entries;
    std::string m_AliasPath;
    std::string m_BasePath;
    bool m_ForceReadOnly = false;
    bool m_IsInitialized = false;
};

} // namespace vfspp

#endif // VFSPP_NATIVEFILESYSTEM_HPP
