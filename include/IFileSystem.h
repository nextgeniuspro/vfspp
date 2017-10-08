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
CLASS_PTR(IFile)

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
    virtual IFilePtr OpenFile(const CFileInfo& filePath, int mode) = 0;
    
    /*
     * Close file
     */
    virtual void CloseFile(IFilePtr file) = 0;
    
    /*
     * Create file on writeable filesystem. Return true if file already exists
     */
    virtual bool CreateFile(const CFileInfo& filePath) = 0;
    
    /*
     * Remove existing file on writable filesystem
     */
    virtual bool RemoveFile(const CFileInfo& filePath) = 0;
    
    /*
     * Copy existing file on writable filesystem
     */
    virtual bool CopyFile(const CFileInfo& src, const CFileInfo& dest) = 0;
    
    /*
     * Rename existing file on writable filesystem
     */
    virtual bool RenameFile(const CFileInfo& src, const CFileInfo& dest) = 0;
    
    /*
     * Check if file exists on filesystem
     */
    virtual bool IsFileExists(const CFileInfo& filePath) const = 0;
    
    /*
     * Check is file
     */
    virtual bool IsFile(const CFileInfo& filePath) const = 0;
    
    /*
     * Check is dir
     */
    virtual bool IsDir(const CFileInfo& dirPath) const = 0;
};


    
}; // namespace vfspp

#endif /* IFILESYSTEM_H */
