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
        if (m_IsInitialized) {
            return;
        }
        m_IsInitialized = true;
    }

    /*
     * Shutdown filesystem
     */
    virtual void Shutdown() override
    {
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
        static std::string basePath = "/";
        return basePath;
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
        return false;
    }
    
    /*
     * Open existing file for reading, if not exists returns null for readonly filesystem. 
     * If file not exists and filesystem is writable then create new file
     */
    virtual IFilePtr OpenFile(const FileInfo& filePath, IFile::FileMode mode) override
    {
        IFilePtr file = FindFile(filePath);
        bool isExists = (file != nullptr);
        if (!isExists && !IsReadOnly()) {
            file.reset(new MemoryFile(filePath));
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
        IFilePtr file = FindFile(filePath);
        if (!file) {
            return false;
        }

        m_FileList.erase(file);
        return true;
    }
    
    /*
     * Copy existing file on writable filesystem
     */
    virtual bool CopyFile(const FileInfo& src, const FileInfo& dest) override
    {
        MemoryFilePtr srcFile = std::static_pointer_cast<MemoryFile>(FindFile(src));
        MemoryFilePtr dstFile = std::static_pointer_cast<MemoryFile>(OpenFile(dest, IFile::FileMode::Write | IFile::FileMode::Truncate));
        
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
    
    /*
     * Rename existing file on writable filesystem
     */
    virtual bool RenameFile(const FileInfo& srcPath, const FileInfo& dstPath) override
    {
        bool result = CopyFile(srcPath, dstPath);
        if (result)  {
            result = RemoveFile(srcPath);
        }
        
        return result;
    }
    
private:
    bool m_IsInitialized;
    TFileList m_FileList;
};

} // namespace vfspp

#endif // MEMORYFILESYSTEM_HPP