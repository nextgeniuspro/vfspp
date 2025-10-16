#ifndef VFSPP_IFILESYSTEM_H
#define VFSPP_IFILESYSTEM_H

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
    [[nodiscard]]
    virtual bool Initialize() = 0;
    /*
     * Shutdown filesystem
     */
    virtual void Shutdown() = 0;
    
    /*
     * Check if filesystem is initialized
     */
    [[nodiscard]]
    virtual bool IsInitialized() const = 0;
    
    /*
     * Get base path
     */
    [[nodiscard]]
    virtual const std::string& BasePath() const = 0;

    /*
    * Get mounted path
    */
    [[nodiscard]]
    virtual const std::string& VirtualPath() const = 0;
    
    /*
     * Retrieve all files in filesystem. Heavy operation, avoid calling this often
     */
    [[nodiscard]]
    virtual FilesList GetFilesList() const = 0;
    
    /*
     * Check is readonly filesystem
     */
    [[nodiscard]]
    virtual bool IsReadOnly() const = 0;
    
    /*
     * Open existing file for reading, if not exists return null
     */
    virtual IFilePtr OpenFile(const std::string& virtualPath, IFile::FileMode mode) = 0;
    
    /*
     * Close file
     */
    virtual void CloseFile(IFilePtr file) = 0;

    /*
     * Create file on writeable filesystem. Return true if file already exists
     */
    virtual IFilePtr CreateFile(const std::string& virtualPath) = 0;
    
    /*
     * Remove existing file on writable filesystem
     */
    virtual bool RemoveFile(const std::string& virtualPath) = 0;
    
    /*
     * Copy existing file on writable filesystem
     */
    virtual bool CopyFile(const std::string& srcVirtualPath, const std::string& dstVirtualPath, bool overwrite = false) = 0;
    
    /*
     * Rename existing file on writable filesystem (Move file)
     */
    virtual bool RenameFile(const std::string& srcVirtualPath, const std::string& dstVirtualPath) = 0;
    
    /*
     * Check if file exists on filesystem
     */
    [[nodiscard]]
    virtual bool IsFileExists(const std::string& virtualPath) const = 0;
};

}; // namespace vfspp

#endif // VFSPP_IFILESYSTEM_H
