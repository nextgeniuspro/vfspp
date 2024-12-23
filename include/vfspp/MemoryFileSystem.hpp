#ifndef MEMORYFILESYSTEM_HPP
#define MEMORYFILESYSTEM_HPP

#include "IFileSystem.h"
#include "Global.h"
#include "StringUtils.hpp"
#include "MemoryFile.hpp"

namespace fs = std::filesystem;

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
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return BasePathST();
        } else {
            return BasePathST();
        }
    }
    
    /*
     * Retrieve file list according filter
     */
    virtual const TFileList& FileList() const override
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
    virtual bool CopyFile(const FileInfo& src, const FileInfo& dest) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return CopyFileST(src, dest);
        } else {
            return CopyFileST(src, dest);
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
        // close all files
        for (auto& file : m_FileList) {
            file.second->Close();
        }
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

    inline const TFileList& FileListST() const
    {
        return m_FileList;
    }

    inline bool IsReadOnlyST() const
    {
        return false;
    }

    inline IFilePtr OpenFileST(const FileInfo& filePath, IFile::FileMode mode)
    {
        IFilePtr file = FindFile(filePath, m_FileList);
        bool isExists = (file != nullptr);
        if (!isExists && !IsReadOnlyST()) {
            file.reset(new MemoryFile(filePath));
        }

        if (file) {
            file->Open(mode);
            
            if (!isExists && file->IsOpened()) {
                m_FileList[filePath.AbsolutePath()] = file;
            }
        }
        
        return file;
    }

    inline void CloseFileST(IFilePtr file)
    {
        if (!file) {
            return;
        }

        file->Close();
        m_FileList.erase(file->GetFileInfo().AbsolutePath());
    }

    inline bool CreateFileST(const FileInfo& filePath)
    {
        IFilePtr file = OpenFileST(filePath, IFile::FileMode::Write | IFile::FileMode::Truncate);
        if (file) {
            file->Close();
            return true;
        }
        
        return false;
    }

    inline bool RemoveFileST(const FileInfo& filePath)
    {
        auto numRemoved = m_FileList.erase(filePath.AbsolutePath());
        return numRemoved > 0;
    }

    inline bool CopyFileST(const FileInfo& src, const FileInfo& dest)
    {
        MemoryFilePtr srcFile = std::static_pointer_cast<MemoryFile>(FindFile(src, m_FileList));
        MemoryFilePtr dstFile = std::static_pointer_cast<MemoryFile>(OpenFileST(dest, IFile::FileMode::Write | IFile::FileMode::Truncate));
        
        if (srcFile && dstFile) {
            bool needClose = false;
            if (!srcFile->IsOpened()) {
                needClose = true;
                srcFile->Open(IFile::FileMode::Read);
            }

            dstFile->m_Data.assign(srcFile->m_Data.begin(), srcFile->m_Data.end());
            dstFile->Close();

            if (needClose) {
                srcFile->Close();
            }

            return true;
        }

        return false;
    }

    inline bool RenameFileST(const FileInfo& srcPath, const FileInfo& dstPath)
    {
        bool result = CopyFileST(srcPath, dstPath);
        if (result)  {
            result = RemoveFileST(srcPath);
        }
        
        return result;
    }

    inline bool IsFileExistsST(const FileInfo& filePath) const
    {
        return FindFile(filePath, m_FileList) != nullptr;
    }
    
private:
    bool m_IsInitialized;
    TFileList m_FileList;
    mutable std::mutex m_Mutex;
};

} // namespace vfspp

#endif // MEMORYFILESYSTEM_HPP