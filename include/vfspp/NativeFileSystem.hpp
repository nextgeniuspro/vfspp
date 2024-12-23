#ifndef NATIVEFILESYSTEM_HPP
#define NATIVEFILESYSTEM_HPP

#include "IFileSystem.h"
#include "Global.h"
#include "StringUtils.hpp"
#include "NativeFile.hpp"

namespace fs = std::filesystem;

namespace vfspp
{

using NativeFileSystemPtr = std::shared_ptr<class NativeFileSystem>;
using NativeFileSystemWeakPtr = std::weak_ptr<class NativeFileSystem>;

class NativeFileSystem final : public IFileSystem
{
public:
    NativeFileSystem(const std::string& basePath)
        : m_BasePath(basePath)
        , m_IsInitialized(false)
    {
    }

    ~NativeFileSystem()
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

        if (!fs::exists(m_BasePath) || !fs::is_directory(m_BasePath)) {
            return;
        }

        BuildFilelist(m_BasePath, m_FileList);
        m_IsInitialized = true;
    }

    inline void ShutdownST()
    {
        m_BasePath = "";
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
        return m_BasePath;
    }

    inline const TFileList& FileListST() const
    {
        return m_FileList;
    }
    
    inline bool IsReadOnlyST() const
    {
        if (!IsInitializedST()) {
            return true;
        }
        
        auto perms = fs::status(m_BasePath).permissions();
        return (perms & fs::perms::owner_write) == fs::perms::none;
    }
    
    inline IFilePtr OpenFileST(const FileInfo& filePath, IFile::FileMode mode)
    {
        // check if filesystem is readonly and mode is write then return null
        bool requestWrite = ((mode & IFile::FileMode::Write) == IFile::FileMode::Write);
        requestWrite |= ((mode & IFile::FileMode::Append) == IFile::FileMode::Append);
        requestWrite |= ((mode & IFile::FileMode::Truncate) == IFile::FileMode::Truncate);

        if (IsReadOnlyST() && requestWrite) {
            return nullptr;
        }

        IFilePtr file = FindFile(filePath, m_FileList);
        bool isExists = (file != nullptr);
        if (!isExists && !IsReadOnlyST()) {
            mode = mode | IFile::FileMode::Truncate;
            file.reset(new NativeFile(filePath));
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
        if (file) {
            file->Close();
        }
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
        if (IsReadOnlyST()) {
            return false;
        }
        
        m_FileList.erase(filePath.AbsolutePath());
        return fs::remove(filePath.AbsolutePath());
    }
    
    inline bool CopyFileST(const FileInfo& src, const FileInfo& dest)
    {
        if (IsReadOnlyST()) {
            return false;
        }
        
        if (!IsFileExistsST(src)) {
            return false;
        }
        
        return fs::copy_file(src.AbsolutePath(), dest.AbsolutePath());
    }
    
    bool RenameFileST(const FileInfo& srcPath, const FileInfo& dstPath)
    {
        if (IsReadOnlyST()) {
            return false;
        }
        
        if (!IsFileExistsST(srcPath)) {
            return false;
        }
        
        fs::rename(srcPath.AbsolutePath(), dstPath.AbsolutePath());
        return true;
    }

    inline bool IsFileExistsST(const FileInfo& filePath) const
    {
        return FindFile(filePath, m_FileList) != nullptr;
    }

    void BuildFilelist(std::string basePath, TFileList& outFileList)
    {
        for (const auto& entry : fs::directory_iterator(basePath)) {
            auto filename = entry.path().filename().string();
            if (fs::is_directory(entry.status())) {
                BuildFilelist(entry.path().string(), outFileList);
            } else if (fs::is_regular_file(entry.status())) {
                FileInfo fileInfo(basePath, filename, false);
                IFilePtr file(new NativeFile(fileInfo));
                outFileList[fileInfo.AbsolutePath()] = file;
            }
        }
    }
    
private:
    std::string m_BasePath;
    bool m_IsInitialized;
    TFileList m_FileList;
    mutable std::mutex m_Mutex;
};

} // namespace vfspp

#endif // NATIVEFILESYSTEM_HPP