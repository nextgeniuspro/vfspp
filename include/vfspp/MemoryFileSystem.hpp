#ifndef MEMORYFILESYSTEM_HPP
#define MEMORYFILESYSTEM_HPP

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
    MemoryFileSystem()
        : m_IsInitialized(false)
    {
    }

    ~MemoryFileSystem()
    {
        Shutdown();
    }
    
    /*
     * Initialize filesystem, call this method as soon as possible
     */
    virtual void Initialize() override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            InitializeST();
        } else {
            InitializeST();
        }
    }

    /*
     * Shutdown filesystem
     */
    virtual void Shutdown() override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            ShutdownST();
        } else {
            ShutdownST();
        }
    }
    
    /*
     * Check if filesystem is initialized
     */
    virtual bool IsInitialized() const override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return IsInitializedST();
        } else {
            return IsInitializedST();
        }
    }
    
    /*
     * Get base path
     */
    virtual const std::string& BasePath() const override
    {
        return BasePathST();
    }
    
    /*
     * Retrieve file list according filter
     */
    virtual const FilesList& FileList() const override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return FileListST();
        } else {
            return FileListST();
        }
    }
    
    /*
     * Check is readonly filesystem
     */
    virtual bool IsReadOnly() const override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return IsReadOnlyST();
        } else {
            return IsReadOnlyST();
        }
    }
    
    /*
     * Open existing file for reading, if not exists returns null for readonly filesystem. 
     * If file not exists and filesystem is writable then create new file
     */
    virtual IFilePtr OpenFile(const FileInfo& filePath, IFile::FileMode mode) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return OpenFileST(filePath, mode);
        } else {
            return OpenFileST(filePath, mode);
        }
    }

    /*
     * Close file
     */
    virtual void CloseFile(IFilePtr file) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            CloseFileST(file);
        } else {
            CloseFileST(file);
        }
    }
    
    /*
     * Create file on writeable filesystem. Returns true if file created successfully
     */
    virtual bool CreateFile(const FileInfo& filePath) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return CreateFileST(filePath);
        } else {
            return CreateFileST(filePath);
        }
    }
    
    /*
     * Remove existing file on writable filesystem
     */
    virtual bool RemoveFile(const FileInfo& filePath) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return RemoveFileST(filePath);
        } else {
            return RemoveFileST(filePath);
        }
    }
    
    /*
     * Copy existing file on writable filesystem
     */
    virtual bool CopyFile(const FileInfo& src, const FileInfo& dest, bool overwrite = false) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return CopyFileST(src, dest, overwrite);
        } else {
            return CopyFileST(src, dest, overwrite);
        }
    }
    
    /*
     * Rename existing file on writable filesystem
     */
    virtual bool RenameFile(const FileInfo& srcPath, const FileInfo& dstPath) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return RenameFileST(srcPath, dstPath);
        } else {
            return RenameFileST(srcPath, dstPath);
        }
    }

    /*
     * Check if file exists on filesystem
     */
    virtual bool IsFileExists(const FileInfo& filePath) const override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return IsFileExistsST(filePath);
        } else {
            return IsFileExistsST(filePath);
        }
    }

    /*
     * Check is file
     */
    virtual bool IsFile(const FileInfo& filePath) const override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return IFileSystem::IsFile(filePath, m_FileList);
        } else {
            return IFileSystem::IsFile(filePath, m_FileList);
        }
    }
    
    /*
     * Check is dir
     */
    virtual bool IsDir(const FileInfo& dirPath) const override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return IFileSystem::IsDir(dirPath, m_FileList);
        } else {
            return IFileSystem::IsDir(dirPath, m_FileList);
        }
    }

private:
    inline void InitializeST()
    {
        if (m_IsInitialized) {
            return;
        }
        m_IsInitialized = true;
    }

    inline void ShutdownST()
    {
        m_Files.clear();
        m_FileList.clear();
        
        m_IsInitialized = false;
    }

    inline bool IsInitializedST() const
    {
        return m_IsInitialized;
    }

    inline const std::string& BasePathST() const
    {
        static std::string basePath = "/";
        return basePath;
    }

    inline const FilesList& FileListST() const
    {
        return m_FileList;
    }

    inline bool IsReadOnlyST() const
    {
        return false;
    }

    inline IFilePtr OpenFileST(const FileInfo& filePath, IFile::FileMode mode)
    {
        auto absolutePath = filePath.AbsolutePath();
        
        auto [entryIt, inserted] = m_Files.try_emplace(absolutePath, filePath, std::make_shared<MemoryFileObject>());
        if (inserted) {
            m_FileList.push_back(filePath);
        }

        auto& entry = entryIt->second;
        if (!entry.Object) {
            entry.Object = std::make_shared<MemoryFileObject>();
        }
        if (!entry.Object) {
            return nullptr;
        }

        MemoryFilePtr file = std::make_shared<MemoryFile>(entry.Info, entry.Object);
        if (!file || !file->Open(mode)) {
            return nullptr;
        }

        entry.OpenedHandles.push_back(file);

        return file;
    }

    inline void CloseFileST(IFilePtr file)
    {
        CloseFileAndCleanupOpenedHandles(file);
    }

    inline bool CreateFileST(const FileInfo& filePath)
    {
        IFilePtr file = OpenFileST(filePath, IFile::FileMode::Write | IFile::FileMode::Truncate);
        CloseFileAndCleanupOpenedHandles(file);
        return file != nullptr;
    }

    inline bool RemoveFileST(const FileInfo& filePath)
    {
        auto absolutePath = filePath.AbsolutePath();
        
        auto it = m_Files.find(absolutePath);
        if (it == m_Files.end()) {
            return false;
        }

        CloseFileAndCleanupOpenedHandles();

        m_Files.erase(it);
        m_FileList.erase(std::remove_if(m_FileList.begin(), m_FileList.end(), [&](const FileInfo& f) {
            return f.AbsolutePath() == absolutePath;
        }), m_FileList.end());

        return true;
    }

    inline bool CopyFileST(const FileInfo& srcPath, const FileInfo& dstPath, bool overwrite = false)
    {
        const auto srcIt = m_Files.find(srcPath.AbsolutePath());
        if (srcIt == m_Files.end()) {
            return false;
        }

        const auto destIt = m_Files.find(dstPath.AbsolutePath());

        // Destination file exists and overwrite is false
        if (destIt != m_Files.end() && !overwrite) {
            return false;
        }

        // Remove existing destination file
        if (destIt != m_Files.end() && overwrite) {
            m_Files.erase(destIt);
            m_FileList.erase(std::remove_if(m_FileList.begin(), m_FileList.end(), [&](const FileInfo& f) {
                return f.AbsolutePath() == dstPath.AbsolutePath();
            }), m_FileList.end());
        }

        MemoryFileObjectPtr newObject = std::make_shared<MemoryFileObject>();
    //        newObject->Update(std::make_shared<std::vector<uint8_t>>(*srcIt->second.Object->GetSnapshot())); // TODO: fix copy
        const auto [destEntryIt, inserted] = m_Files.insert_or_assign(dstPath.AbsolutePath(), MemoryFileSystem::FileEntry(dstPath, newObject));
        if (inserted) {
            m_FileList.push_back(dstPath);
        }

        return true;
    }

    inline bool RenameFileST(const FileInfo& srcPath, const FileInfo& dstPath)
    {
        bool result = CopyFileST(srcPath, dstPath, false);
        if (result)  {
            result = RemoveFileST(srcPath);
        }
        
        return result;
    }

    inline bool IsFileExistsST(const FileInfo& filePath) const
    {
        return m_Files.find(filePath.AbsolutePath()) != m_Files.end();
    }

    inline void CloseFileAndCleanupOpenedHandles(IFilePtr fileToClose = nullptr)
    {
        if (fileToClose) {
            const auto absolutePath = fileToClose->GetFileInfo().AbsolutePath();
            
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
    bool m_IsInitialized;
    mutable std::mutex m_Mutex;

    struct FileEntry
    {
        FileInfo Info;
        MemoryFileObjectPtr Object;
        std::vector<MemoryFileWeakPtr> OpenedHandles;

        FileEntry(const FileInfo& info, MemoryFileObjectPtr object)
            : Info(info)
            , Object(object)
        {
        }

        void CleanupOpenedHandles(IFilePtr fileToExclude = nullptr)
        {
            OpenedHandles.erase(std::remove_if(OpenedHandles.begin(), OpenedHandles.end(), [&](const MemoryFileWeakPtr& weak) {
                return weak.expired() || weak.lock() == fileToExclude;
            }), OpenedHandles.end());
        }
    };

    std::unordered_map<std::string, FileEntry> m_Files;
    FilesList m_FileList;
};

} // namespace vfspp

#endif // MEMORYFILESYSTEM_HPP
