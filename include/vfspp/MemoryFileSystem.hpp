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
        m_Files.clear();
        
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
        return false;
    }

    inline IFilePtr OpenFileImpl(const std::string& virtualPath, IFile::FileMode mode)
    {
        FileInfo fileInfo(AliasPathImpl(), BasePathImpl(), virtualPath);

        auto [entryIt, inserted] = m_Files.try_emplace(virtualPath, fileInfo, std::make_shared<MemoryFileObject>());
        if (!inserted) {
            return nullptr;
        }

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
        auto it = m_Files.find(virtualPath);
        if (it == m_Files.end()) {
            return false;
        }

        CloseFileAndCleanupOpenedHandles();

        m_Files.erase(it);

        return true;
    }

    inline bool CopyFileImpl(const std::string& srcVirtualPath, const std::string& dstVirtualPath, bool overwrite = false)
    {
        const auto srcIt = m_Files.find(srcVirtualPath);
        if (srcIt == m_Files.end()) {
            return false;
        }

        const auto destIt = m_Files.find(dstVirtualPath);

        // Destination file exists and overwrite is false
        if (destIt != m_Files.end() && !overwrite) {
            return false;
        }

        // Remove existing destination file
        if (destIt != m_Files.end()) {
            m_Files.erase(destIt);
        }

        // Create copy of memory object
        MemoryFileObjectPtr newObject;
        if (srcIt->second.Object) {
            newObject = std::make_shared<MemoryFileObject>(*srcIt->second.Object);
        } else {
            newObject = std::make_shared<MemoryFileObject>();
        }

        FileInfo fileInfo(AliasPathImpl(), BasePathImpl(), dstVirtualPath);
        m_Files.insert({dstVirtualPath, FileEntry(fileInfo, std::move(newObject))});

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

    inline bool IsFileExistsImpl(const std::string& virtualPath) const
    {
        return m_Files.find(virtualPath) != m_Files.end();
    }

    inline void CloseFileAndCleanupOpenedHandles(IFilePtr fileToClose = nullptr)
    {
        if (fileToClose) {
            const auto absolutePath = fileToClose->GetFileInfo().VirtualPath();
            
            const auto it = m_Files.find(absolutePath);
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
    bool m_IsInitialized = false;
    mutable std::mutex m_Mutex;

    struct FileEntry
    {
        FileInfo Info;
        MemoryFileObjectPtr Object;
        using WeakHandle = MemoryFileWeakPtr;
        std::vector<WeakHandle> OpenedHandles;

        FileEntry(const FileInfo& info, MemoryFileObjectPtr object)
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

    std::unordered_map<std::string, FileEntry> m_Files;
};

} // namespace vfspp

#endif // VFSPP_MEMORYFILESYSTEM_HPP
