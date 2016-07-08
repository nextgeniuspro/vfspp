//
//  CVirtualFileSystem.h
//  vfspp
//
//  Created by Yevgeniy Logachev on 6/23/16.
//
//

#ifndef CVIRTUALFILESYSTEM_H
#define CVIRTUALFILESYSTEM_H

#include "IFileSystem.h"
#include "IFile.h"

namespace vfspp
{
CLASS_PTR(IFile)
CLASS_PTR(IFileSystem)
CLASS_PTR(CVirtualFileSystem)
    
extern void vfs_initialize();
extern void vfs_shutdown();
extern CVirtualFileSystemPtr vfs_get_global();
    
class CVirtualFileSystem final
{
public:
    typedef std::list<IFileSystemPtr> TFileSystemList;
    typedef std::unordered_map<std::string, IFileSystemPtr> TFileSystemMap;
    
    struct SSortedAlias
    {
        std::string alias;
        IFileSystemPtr filesystem;
        
        SSortedAlias(const std::string& a,
                     IFileSystemPtr fs)
        : alias(a)
        , filesystem(fs)
        {}
    };
    typedef std::list<SSortedAlias> TSortedAliasList;
    
public:
    CVirtualFileSystem();
    ~CVirtualFileSystem();
    
    /*
     * Register new filesystem. Alias is base prefix to file access.
     * For ex. registered filesystem has base path '/home/media', but registered
     * with alias '/', so it possible to access files with path '/filename'
     * instead of '/home/media/filename
     */
    void AddFileSystem(const std::string& alias, IFileSystemPtr filesystem);
    
    /*
     * Remove registered filesystem
     */
    void RemoveFileSystem(const std::string& alias);
    
    /*
     * Check if filesystem with 'alias' registered
     */
    bool IsFileSystemExists(const std::string& alias) const;
    
    /*
     * Get filesystem with 'alias'
     */
    IFileSystemPtr GetFilesystem(const std::string& alias);
    
    /*
     * Iterate over all registered filesystems and find first ocurrences of file
     */
    IFilePtr OpenFile(const CFileInfo& filePath, IFile::FileMode mode);
    
    /*
     * Close opened file if it was opened via OpenFirstFile(..)
     */
    void CloseFile(IFilePtr file);
    
private:
    TFileSystemMap m_FileSystem;
    TSortedAliasList m_SortedAlias;
    std::unordered_map<uintptr_t, IFileSystemPtr> m_OpenedFiles;
};
    
}; // namespace vfspp

#endif /* CVIRTUALFILESYSTEM_H */
