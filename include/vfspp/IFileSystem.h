//
//  IFileSystem.h
//  vfspp
//
//  Created by Yevgeniy Logachev on 6/22/16.
//
//

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
    typedef std::set<IFilePtr> TFileList;
    
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
    virtual const TFileList& FileList() const = 0;
    
    /*
     * Check is readonly filesystem
     */
    virtual bool IsReadOnly() const = 0;
    
    /*
     * Open existing file for reading, if not exists return null
     */
    virtual IFilePtr OpenFile(const FileInfo& filePath, int mode) = 0;
    
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
    virtual bool CopyFile(const FileInfo& src, const FileInfo& dest) = 0;
    
    /*
     * Rename existing file on writable filesystem
     */
    virtual bool RenameFile(const FileInfo& src, const FileInfo& dest) = 0;
    
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
};

}; // namespace vfspp

#endif /* IFILESYSTEM_H */
