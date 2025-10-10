#ifndef IFILESYSTEM_H
#define IFILESYSTEM_H

#include "IFile.h"

namespace vfspp
{

using IFileSystemPtr = std::shared_ptr<class IFileSystem>;
using IFileSystemWeakPtr = std::weak_ptr<class IFileSystem>;

class IFileSystem
{
public:
    using FilesList = std::vector<FileInfo>;
    
public:
    IFileSystem() = default;
    ~IFileSystem() = default;
    
    /*
     * Initialize filesystem, call this method as soon as possible
     */
    virtual void Initialize() = 0;
    /*
     * Shutdown filesystem
     */
    virtual void Shutdown() = 0;
    
    /*
     * Check if filesystem is initialized
     */
    virtual bool IsInitialized() const = 0;
    
    /*
     * Get base path
     */
    virtual const std::string& BasePath() const = 0;
    
    /*
     * Retrieve file list according filter
     */
    virtual const FilesList& FileList() const = 0;
    
    /*
     * Check is readonly filesystem
     */
    virtual bool IsReadOnly() const = 0;
    
    /*
     * Open existing file for reading, if not exists return null
     */
    virtual IFilePtr OpenFile(const FileInfo& filePath, IFile::FileMode mode) = 0;
    
    /*
     * Close file
     */
    virtual void CloseFile(IFilePtr file) = 0;

    /*
     * Create file on writeable filesystem. Return true if file already exists
     */
    virtual bool CreateFile(const FileInfo& filePath) = 0;
    
    /*
     * Remove existing file on writable filesystem
     */
    virtual bool RemoveFile(const FileInfo& filePath) = 0;
    
    /*
     * Copy existing file on writable filesystem
     */
    virtual bool CopyFile(const FileInfo& src, const FileInfo& dest, bool overwrite = false) = 0;
    
    /*
     * Rename existing file on writable filesystem (Move file)
     */
    virtual bool RenameFile(const FileInfo& src, const FileInfo& dest, bool overwrite = false) = 0;
    
    /*
     * Check if file exists on filesystem
     */
    virtual bool IsFileExists(const FileInfo& filePath) const = 0;
    
    /*
     * Check is file
     */
    virtual bool IsFile(const FileInfo& filePath) const = 0;
    
    /*
     * Check is dir
     */
    virtual bool IsDir(const FileInfo& dirPath) const = 0;

protected:
    inline bool IsFile(const FileInfo& filePath, const FilesList& fileList) const
    {
        auto fileInfo = FindFileInfo(filePath, fileList);
        if (fileInfo) {
            return !fileInfo.value().IsDir();
        }
        return false;
    }
    
    inline bool IsDir(const FileInfo& dirPath, const FilesList& fileList) const
    {
        return !IsFile(dirPath, fileList);
    }

    std::optional<FileInfo> FindFileInfo(const FileInfo& fileInfo, const FilesList& fileList) const
    {
        auto it = std::find_if(fileList.begin(), fileList.end(), [&fileInfo](const FileInfo& f) {
            return (f.AbsolutePath() == fileInfo.AbsolutePath());
        });
        if (it != fileList.end()) {
            return std::optional(*it);
        }
        return std::nullopt;
    }
};

}; // namespace vfspp

#endif // IFILESYSTEM_H
