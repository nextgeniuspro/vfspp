#ifndef NATIVEFILESYSTEM_HPP
#define NATIVEFILESYSTEM_HPP

#include "IFileSystem.h"
#include "VFS.h"
#include "StringUtils.hpp"
#include "NativeFile.hpp"

namespace fs = std::filesystem;

namespace vfspp
{

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
    virtual IFilePtr OpenFile(const FileInfo& filePath, int mode) override
    {
        // check if filesystem is readonly and mode is write then return null
        int writeMask = static_cast<int>(IFile::FileMode::Write) | static_cast<int>(IFile::FileMode::ReadWrite) | static_cast<int>(IFile::FileMode::Append) | static_cast<int>(IFile::FileMode::Truncate);
        if (IsReadOnly() && (mode & writeMask)) {
            return nullptr;
        }

        IFilePtr file = FindFile(filePath);
        bool isExists = (file != nullptr);
        if (!isExists && !IsReadOnly()) {
            mode |= static_cast<int>(IFile::FileMode::Truncate);
            file.reset(new NativeFile(filePath));
        }
        file->Open(mode);
        
        if (!isExists && file->IsOpened()) {
            m_FileList.insert(file);
        }
        
        return file;
    }
    
    /*
     * Close file
     */
    virtual void CloseFile(IFilePtr file) override
    {
        if (!file) {
            return;
        }
        file->Close();
    }
    
    /*
     * Create file on writeable filesystem. Returns true if file created successfully
     */
    virtual bool CreateFile(const FileInfo& filePath) override
    {
        IFilePtr file = OpenFile(filePath, static_cast<int>(IFile::FileMode::Write) | static_cast<int>(IFile::FileMode::Truncate));
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
    
    /*
     * Check if file exists on filesystem
     */
    virtual bool IsFileExists(const FileInfo& filePath) const override
    {
        return fs::exists(filePath.AbsolutePath());
    }
    
    /*
     * Check is file
     */
    virtual bool IsFile(const FileInfo& filePath) const override
    {
        IFilePtr file = FindFile(filePath);
        if (file) {
            return !file->GetFileInfo().IsDir();
        }
        return false;
    }
    
    /*
     * Check is dir
     */
    virtual bool IsDir(const FileInfo& dirPath) const override
    {
        IFilePtr file = FindFile(dirPath);
        if (file) {
            return file->GetFileInfo().IsDir();
        }
        return false;
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

    IFilePtr FindFile(const FileInfo& fileInfo) const
    {
        TFileList::const_iterator it = std::find_if(m_FileList.begin(), m_FileList.end(), [&](IFilePtr file) {
            return file->GetFileInfo() == fileInfo;
        });
        
        if (it != m_FileList.end()) {
            return *it;
        }
        
        return nullptr;
    }
    
private:
    std::string m_BasePath;
    bool m_IsInitialized;
    TFileList m_FileList;
};

} // namespace vfspp

#endif // NATIVEFILESYSTEM_HPP