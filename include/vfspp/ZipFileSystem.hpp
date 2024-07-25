#ifndef ZIPFILESYSTEM_HPP
#define ZIPFILESYSTEM_HPP

#include "IFileSystem.h"
#include "Global.h"
#include "StringUtils.hpp"
#include "ZipFile.hpp"
#include "zip_file.hpp"

namespace fs = std::filesystem;

namespace vfspp
{

using ZipFileSystemPtr = std::shared_ptr<class ZipFileSystem>;
using ZipFileSystemWeakPtr = std::weak_ptr<class ZipFileSystem>;


class ZipFileSystem final : public IFileSystem
{
public:
    ZipFileSystem(const std::string& zipPath)
        : m_ZipPath(zipPath)
        , m_ZipArchive(nullptr)
        , m_IsInitialized(false)
    {
    }

    ~ZipFileSystem()
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

        if (!fs::is_regular_file(m_ZipPath)) {
            return;
        }

        m_ZipArchive = std::make_shared<mz_zip_archive>();

        mz_bool status = mz_zip_reader_init_file(m_ZipArchive.get(), m_ZipPath.c_str(), 0);
        if (!status) {
            return;
        }

        BuildFilelist(m_ZipArchive, m_FileList);
        m_IsInitialized = true;
    }

    /*
     * Shutdown filesystem
     */
    virtual void Shutdown() override
    {
        m_ZipPath = "";
        // close all files
        for (auto& file : m_FileList) {
            file->Close();
        }
        m_FileList.clear();

        // close zip archive
        if (m_ZipArchive) {
            mz_zip_reader_end(m_ZipArchive.get());
            m_ZipArchive = nullptr;
        }

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
        static std::string rootPath = "/";
        return rootPath;
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
        return true;
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
        if (file) {
            file->Open(mode);
        }
        
        return file;
    }
    
    /*
     * Create file on writeable filesystem. Returns true if file created successfully
     */
    virtual bool CreateFile(const FileInfo& filePath) override
    {
        return false;
    }
    
    /*
     * Remove existing file on writable filesystem
     */
    virtual bool RemoveFile(const FileInfo& filePath) override
    {
        return false;
    }
    
    /*
     * Copy existing file on writable filesystem
     */
    virtual bool CopyFile(const FileInfo& src, const FileInfo& dest) override
    {
        return false;
    }
    
    /*
     * Rename existing file on writable filesystem
     */
    virtual bool RenameFile(const FileInfo& srcPath, const FileInfo& dstPath) override
    {
        return false;
    }

private:
    void BuildFilelist(std::shared_ptr<mz_zip_archive> zipArchive, TFileList& outFileList)
    {
        for (mz_uint i = 0; i < mz_zip_reader_get_num_files(zipArchive.get()); i++) {
            mz_zip_archive_file_stat file_stat;
            if (!mz_zip_reader_file_stat(zipArchive.get(), i, &file_stat)) {
                // TODO: log error
                continue;
            }
            
            FileInfo fileInfo(BasePath(), file_stat.m_filename, false);
            IFilePtr file(new ZipFile(fileInfo, file_stat.m_file_index, file_stat.m_uncomp_size, zipArchive));
            outFileList.insert(file);
        }
    }
    
private:
    std::string m_ZipPath;
    std::shared_ptr<mz_zip_archive> m_ZipArchive;
    bool m_IsInitialized;
    TFileList m_FileList;
};

} // namespace vfspp

#endif // ZIPFILESYSTEM_HPP