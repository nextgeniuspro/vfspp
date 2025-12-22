#ifndef VFSPP_NATIVEFILESYSTEM_HPP
#define VFSPP_NATIVEFILESYSTEM_HPP

#include "IFileSystem.h"
#include "Global.h"
#include "ThreadingPolicy.hpp"
#include "NativeFile.hpp"

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
    NativeFileSystem(const std::string& aliasPath, const std::string& basePath)
        : m_AliasPath(aliasPath)
        , m_BasePath(basePath)
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
    virtual FilesList GetFilesList() const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return GetFilesListImpl();
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

    /*
     * Check if file exists on filesystem
     */
    [[nodiscard]]
    virtual bool IsFileExists(const std::string& virtualPath) const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return IsFileExistsImpl(virtualPath);
    }

private:
    struct FileEntry
    {
        FileInfo Info;
        std::vector<NativeFileWeakPtr> OpenedHandles;

        explicit FileEntry(const FileInfo& info)
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

        if (!fs::is_directory(BasePathImpl())) {
            printf("NativeFileSystem: Base path is not a directory: %s\n", BasePathImpl().c_str());
            return false;
        }

        if (!fs::exists(BasePathImpl())) { // TODO: create if not exists flag
            printf("NativeFileSystem: Base path does not exist: %s\n", BasePathImpl().c_str());
            return false;
        }

        BuildFilelist(AliasPathImpl(), BasePathImpl(), m_Files);
        m_IsInitialized = true;
        return true;
    }

    inline void ShutdownImpl()
    {
        m_BasePath = "";
        m_AliasPath = "";
        m_Files.clear();

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
    
    inline FilesList GetFilesListImpl() const
    {
        FilesList list;
        list.reserve(m_Files.size());
        for (const auto& [path, entry] : m_Files) {
            list.push_back(entry.Info);
        }
        return list;
    }
    
    inline bool IsReadOnlyImpl() const
    {
        if (!IsInitializedImpl()) {
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

        const auto it = m_Files.find(virtualPath);
        if (it == m_Files.end() && requestWrite) {
            // Create new file entry if not exists in writable mode
            FileInfo fileInfo(AliasPathImpl(), BasePathImpl(), virtualPath);
            m_Files.emplace(fileInfo.VirtualPath(), fileInfo);
        }

        const auto entryIt = m_Files.find(virtualPath);
        if (entryIt == m_Files.end()) {
            return nullptr;
        }
        auto& entry = entryIt->second;

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
        
        auto it = m_Files.find(virtualPath);
        if (it == m_Files.end()) {
            return false;
        }

        CloseFileAndCleanupOpenedHandles();

        m_Files.erase(it);
        
        return fs::remove(it->second.Info.NativePath());
    }

    inline bool CopyFileImpl(const std::string& srcVirtualPath, const std::string& dstVirtualPath, bool overwrite = false)
    {
        if (IsReadOnlyImpl()) {
            return false;
        }

        // Check is src and dst start with alias path
        if (srcVirtualPath.find(AliasPathImpl()) != 0 || dstVirtualPath.find(AliasPathImpl()) != 0) {
            return false;
        }
        
        // Check if src file exists
        const auto srcIt = m_Files.find(srcVirtualPath);
        if (srcIt == m_Files.end()) {
            return false;
        }
        const auto& srcPath = srcIt->second.Info;

        // Check if dst file exists
        const auto dstIt = m_Files.find(dstVirtualPath);
        if (dstIt != m_Files.end() && !overwrite) {
            return false;
        }
        const auto& dstPath = FileInfo(AliasPathImpl(), BasePathImpl(), dstVirtualPath);

        // Perform copy
        auto option = overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::skip_existing;       
        const bool ok = fs::copy_file(srcPath.NativePath(), dstPath.NativePath(), option);
        if (!ok) {
            return false;
        }

        // Remove existing dest entry if any
        const auto destIt = m_Files.find(dstPath.VirtualPath());
        if (destIt != m_Files.end()) {
            m_Files.erase(destIt);
        }

        // Add new entry
        m_Files.insert({dstVirtualPath, FileEntry(dstPath)});
        return true;
    }
    
    bool RenameFileImpl(const std::string& srcVirtualPath, const std::string& dstVirtualPath)
    {
        if (IsReadOnlyImpl()) {
            return false;
        }
        
        // Check is src and dst start with alias path
        if (srcVirtualPath.find(AliasPathImpl()) != 0 || dstVirtualPath.find(AliasPathImpl()) != 0) {
            return false;
        }

        // Check if src file exists
        const auto srcIt = m_Files.find(srcVirtualPath);
        if (srcIt == m_Files.end()) {
            return false;
        }
        const auto& srcPath = srcIt->second.Info;

        // Check if dst file exists
        const auto dstIt = m_Files.find(dstVirtualPath);
        if (dstIt != m_Files.end()) {
            return false;
        }
        const auto& dstPath = FileInfo(AliasPathImpl(), BasePathImpl(), dstVirtualPath);
        
        // Perform rename
        std::error_code ec;
        fs::rename(srcPath.NativePath(), dstPath.NativePath(), ec);
        if (ec) {
            return false;
        }

        // Remove existing src entry
        if (srcIt != m_Files.end()) {
            m_Files.erase(srcIt);
        }

        // Remove existing dest entry if any
        if (dstIt != m_Files.end()) {
            m_Files.erase(dstIt);
        }

        // Add new entry
        m_Files.insert({dstVirtualPath, FileEntry(dstPath)});
        return true;
    }

    inline bool IsFileExistsImpl(const std::string& virtualPath) const
    {
        const auto fileIt = m_Files.find(virtualPath);
        return fileIt != m_Files.end() && fs::exists(fileIt->second.Info.NativePath());
    }

    void BuildFilelist(const std::string& aliasPath, const std::string& basePath, std::unordered_map<std::string, FileEntry>& outFiles)
    {
        for (const auto& entry : fs::directory_iterator(basePath)) {
            if (fs::is_directory(entry.status())) {
                BuildFilelist(aliasPath, entry.path().string(), outFiles);
                continue;
            } 
            
            FileInfo fileInfo(aliasPath, BasePathImpl(), entry.path().string());                
            outFiles.emplace(fileInfo.VirtualPath(), fileInfo);
        }
    }

    inline void CloseFileAndCleanupOpenedHandles(IFilePtr fileToClose = nullptr)
    {
        if (fileToClose) {
            const auto virtualPath = fileToClose->GetFileInfo().VirtualPath();

            const auto it = m_Files.find(virtualPath);
            if (it == m_Files.end()) {
                return;
            }
            
            fileToClose->Close();
        }

        for (auto& [path, entry] : m_Files) {
            entry.CleanupOpenedHandles(fileToClose);
        }
    }
    
private:    
    std::string m_AliasPath;
    std::string m_BasePath;
    bool m_IsInitialized = false;
    mutable std::mutex m_Mutex;

    std::unordered_map<std::string, FileEntry> m_Files;
};

} // namespace vfspp

#endif // VFSPP_NATIVEFILESYSTEM_HPP
