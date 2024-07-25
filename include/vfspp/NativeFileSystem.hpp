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
        if (m_IsInitialized) {
            return;
        }

        if (!fs::exists(m_BasePath) || !fs::is_directory(m_BasePath)) {
            return;
        }

        BuildFilelist(m_BasePath, m_FileList);
        m_IsInitialized = true;
    }

    /*
     * Shutdown filesystem
     */
    virtual void Shutdown() override
    {
        m_BasePath = "";
        // close all files
        for (auto& file : m_FileList) {
            file->Close();
        }
        m_FileList.clear();
        m_IsInitialized = false;
    }
    
    /*
     * Check if filesystem is initialized
     */
    virtual bool IsInitialized() const override
    {
        return m_IsInitialized;
    }
    
    /*
     * Get base path
     */
    virtual const std::string& BasePath() const override
    {
        return m_BasePath;
    }
    
    /*
     * Retrieve file list according filter
     */
    virtual const TFileList& FileList() const override
    {
        return m_FileList;
    }
    
    /*
     * Check is readonly filesystem
     */
    virtual bool IsReadOnly() const override
    {
        if (!IsInitialized()) {
            return true;
        }
        
        auto perms = fs::status(m_BasePath).permissions();
        return (perms & fs::perms::owner_write) == fs::perms::none;
    }
    
    /*
     * Open existing file for reading, if not exists returns null for readonly filesystem. 
     * If file not exists and filesystem is writable then create new file
     */
    virtual IFilePtr OpenFile(const FileInfo& filePath, IFile::FileMode mode) override
    {
        // check if filesystem is readonly and mode is write then return null
        bool requestWrite = ((mode & IFile::FileMode::Write) == IFile::FileMode::Write);
        requestWrite |= ((mode & IFile::FileMode::Append) == IFile::FileMode::Append);
        requestWrite |= ((mode & IFile::FileMode::Truncate) == IFile::FileMode::Truncate);

        if (IsReadOnly() && requestWrite) {
            return nullptr;
        }

        IFilePtr file = FindFile(filePath);
        bool isExists = (file != nullptr);
        if (!isExists && !IsReadOnly()) {
            mode = mode | IFile::FileMode::Truncate;
            file.reset(new NativeFile(filePath));
        }
        file->Open(mode);
        
        if (!isExists && file->IsOpened()) {
            m_FileList.insert(file);
        }
        
        return file;
    }
    
    /*
     * Create file on writeable filesystem. Returns true if file created successfully
     */
    virtual bool CreateFile(const FileInfo& filePath) override
    {
        IFilePtr file = OpenFile(filePath, IFile::FileMode::Write | IFile::FileMode::Truncate);
        if (file) {
            file->Close();
            return true;
        }
        
        return false;
    }
    
    /*
     * Remove existing file on writable filesystem
     */
    virtual bool RemoveFile(const FileInfo& filePath) override
    {
        if (IsReadOnly()) {
            return false;
        }
        
        IFilePtr file = FindFile(filePath);
        if (!file) {
            return false;
        }

        m_FileList.erase(file);
        return fs::remove(filePath.AbsolutePath());
    }
    
    /*
     * Copy existing file on writable filesystem
     */
    virtual bool CopyFile(const FileInfo& src, const FileInfo& dest) override
    {
        if (IsReadOnly()) {
            return false;
        }
        
        if (!IsFileExists(src)) {
            return false;
        }
        
        return fs::copy_file(src.AbsolutePath(), dest.AbsolutePath());
    }
    
    /*
     * Rename existing file on writable filesystem
     */
    virtual bool RenameFile(const FileInfo& srcPath, const FileInfo& dstPath) override
    {
        if (IsReadOnly()) {
            return false;
        }
        
        if (!IsFileExists(srcPath)) {
            return false;
        }
        
        fs::rename(srcPath.AbsolutePath(), dstPath.AbsolutePath());
        return true;
    }

private:
    void BuildFilelist(std::string basePath, TFileList& outFileList)
    {
        for (const auto& entry : fs::directory_iterator(basePath)) {
            auto filename = entry.path().filename().string();
            if (fs::is_directory(entry.status())) {
                BuildFilelist(entry.path().string(), outFileList);
            } else if (fs::is_regular_file(entry.status())) {
                FileInfo fileInfo(basePath, filename, false);
                IFilePtr file(new NativeFile(fileInfo));
                outFileList.insert(file);
            }
        }
    }
    
private:
    std::string m_BasePath;
    bool m_IsInitialized;
    TFileList m_FileList;
};

} // namespace vfspp

#endif // NATIVEFILESYSTEM_HPP