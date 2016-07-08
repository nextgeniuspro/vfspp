//
//  CNativeFileSystem.h
//  vfspp
//
//  Created by Yevgeniy Logachev on 6/23/16.
//
//

#ifndef CNATIVEFILESYSTEM_H
#define CNATIVEFILESYSTEM_H

#include "IFileSystem.h"

struct SDir;

namespace vfspp
{
CLASS_PTR(CNativeFile)
    
class CNativeFileSystem final : public IFileSystem
{
public:
    /*
     * Constructor, create with a base path
     */
    CNativeFileSystem(const std::string& basePath);
    ~CNativeFileSystem();
    /*
     * Initialize filesystem
     */
    virtual void Initialize() override;
    /*
     * Shutdown filesystem
     */
    virtual void Shutdown() override;
    
    /*
     * Check if filesystem is initialized
     */
    virtual bool IsInitialized() const override;
    
    /*
     * Get base path
     */
    virtual const std::string& BasePath() const override;
    
    /*
     * Retrieve file list according filter
     */
    virtual const TFileList& FileList() const override;
    
    /*
     * Check is readonly filesystem
     */
    virtual bool IsReadOnly() const override;
    
    /*
     * Open existing file for reading, if not exists return null
     */
    virtual IFilePtr OpenFile(const CFileInfo& filePath, int mode) override;
    
    /*
     * Close file
     */
    virtual void CloseFile(IFilePtr file) override;
    
    /*
     * Create file on writeable filesystem. Return true if file already exists
     */
    virtual bool CreateFile(const CFileInfo& filePath) override;
    
    /*
     * Remove existing file on writable filesystem
     */
    virtual bool RemoveFile(const CFileInfo& filePath) override;
    
    /*
     * Copy existing file on writable filesystem
     */
    virtual bool CopyFile(const CFileInfo& src, const CFileInfo& dest) override;
    
    /*
     * Rename existing file on writable filesystem
     */
    virtual bool RenameFile(const CFileInfo& src, const CFileInfo& dest) override;
    
    /*
     * Check if file exists on filesystem
     */
    virtual bool IsFileExists(const CFileInfo& filePath) const override;
    
    /*
     * Check is file
     */
    virtual bool IsFile(const CFileInfo& filePath) const override;
    
    /*
     * Check is dir
     */
    virtual bool IsDir(const CFileInfo& dirPath) const override;
    
private:
    IFilePtr FindFile(const CFileInfo& fileInfo) const;
    void BuildFilelist(SDir* dir, std::string basePath, TFileList& outFileList);
    
private:
    std::string m_BasePath;
    bool m_IsInitialized;
    TFileList m_FileList;
};
    
} // namespace vfspp

#endif /* CNATIVEFILESYSTEM_H */
