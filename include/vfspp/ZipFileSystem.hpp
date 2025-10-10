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
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            InitializeST();
        } else {
            InitializeST();
        }
    }

    /*
     * Shutdown filesystem
     */
    virtual void Shutdown() override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            ShutdownST();
        } else {
            ShutdownST();
        }
    }
    
    /*
     * Check if filesystem is initialized
     */
    virtual bool IsInitialized() const override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return IsInitializedST();
        } else {
            return IsInitializedST();
        }
    }
    
    /*
     * Get base path
     */
    virtual const std::string& BasePath() const override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return BasePathST();
        } else {
            return BasePathST();
        }
    }
    
    /*
     * Retrieve file list according filter
     */
    virtual const FilesList& FileList() const override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return FileListST();
        } else {
            return FileListST();
        }
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
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return OpenFileST(filePath, mode);
        } else {
            return OpenFileST(filePath, mode);
        }
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
    virtual bool CopyFile(const FileInfo& src, const FileInfo& dest, bool overwrite = false) override
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
    /*
    * Close file 
    */
    virtual void CloseFile(IFilePtr file) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            CloseFileST(file);
        } else {
            CloseFileST(file);
        }
    }
    /*
     * Check if file exists on filesystem
     */
    virtual bool IsFileExists(const FileInfo& filePath) const override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return IsFileExistsST(filePath);
        } else {
            return IsFileExistsST(filePath);
        }
    }

    /*
     * Check is file
     */
    virtual bool IsFile(const FileInfo& filePath) const override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return IFileSystem::IsFile(filePath, m_FileList);
        } else {
            return IFileSystem::IsFile(filePath, m_FileList);
        }
    }
    
    /*
     * Check is dir
     */
    virtual bool IsDir(const FileInfo& dirPath) const override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return IFileSystem::IsDir(dirPath, m_FileList);
        } else {
            return IFileSystem::IsDir(dirPath, m_FileList);
        }
    }

private:
    struct FileEntry
    {
        FileInfo Info;
        std::vector<ZipFileWeakPtr> OpenedHandles;

        uint32_t EntryID;
        uint64_t Size;

        explicit FileEntry(const FileInfo& info, uint32_t entryID, uint64_t size)
            : Info(info)
            , EntryID(entryID)
            , Size(size)
        {
        }

        void CleanupOpenedHandles(IFilePtr fileToExclude = nullptr)
        {
            OpenedHandles.erase(std::remove_if(OpenedHandles.begin(), OpenedHandles.end(), [&](const ZipFileWeakPtr& weak) {
                return weak.expired() || weak.lock() == fileToExclude;
            }), OpenedHandles.end());
        }
    };

    inline void InitializeST()
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

        BuildFilelist(m_ZipArchive, m_FileList, m_Files);
        m_IsInitialized = true;
    }

    inline void ShutdownST()
    {
        m_ZipPath = "";
        m_Files.clear();
        m_FileList.clear();

        // close zip archive
        if (m_ZipArchive) {
            mz_zip_reader_end(m_ZipArchive.get());
            m_ZipArchive = nullptr;
        }

        m_IsInitialized = false;
    }
    
    inline bool IsInitializedST() const
    {
        return m_IsInitialized;
    }

    inline const std::string& BasePathST() const
    {
        static std::string rootPath = "/";
        return rootPath;
    }

    inline const FilesList& FileListST() const
    {
        return m_FileList;
    }
    
    inline IFilePtr OpenFileST(const FileInfo& filePath, IFile::FileMode mode)
    {
        auto absolutePath = filePath.AbsolutePath();

        const auto entryIt = m_Files.find(absolutePath);
        if (entryIt == m_Files.end()) {
            return nullptr;
        }
        auto& entry = entryIt->second;

        ZipFilePtr file = std::make_shared<ZipFile>(entry.Info, entry.EntryID, entry.Size, m_ZipArchive);
        if (!file || !file->Open(mode)) {
            return nullptr;
        }

        entry.OpenedHandles.push_back(file);
        
        return file;
    }

    inline void CloseFileST(IFilePtr file)
    {
        CloseFileAndCleanupOpenedHandles(file);
    }

    inline bool IsFileExistsST(const FileInfo& filePath) const
    {
        return m_Files.find(filePath.AbsolutePath()) != m_Files.end();
    }

    void BuildFilelist(std::shared_ptr<mz_zip_archive> zipArchive, FilesList& outFileList, std::unordered_map<std::string, FileEntry>& outFiles)
    {
        for (mz_uint i = 0; i < mz_zip_reader_get_num_files(zipArchive.get()); i++) {
            mz_zip_archive_file_stat file_stat;
            if (!mz_zip_reader_file_stat(zipArchive.get(), i, &file_stat)) {
                // TODO: log error
                continue;
            }
            
            FileInfo fileInfo(BasePathST(), file_stat.m_filename, false);
            
            outFileList.push_back(fileInfo);
            outFiles.emplace(
                fileInfo.AbsolutePath(),
                FileEntry(
                    fileInfo,
                    static_cast<uint32_t>(file_stat.m_file_index),
                    static_cast<uint64_t>(file_stat.m_uncomp_size)
                )
            );
        }
    }

    inline void CloseFileAndCleanupOpenedHandles(IFilePtr fileToClose = nullptr)
    {
        if (fileToClose) {
            const auto absolutePath = fileToClose->GetFileInfo().AbsolutePath();
            
            const auto it = m_Files.find(absolutePath);
            if (it == m_Files.end()) {
                return;
            }
            
            fileToClose->Close();
        }

        for (auto& [path, entry] : m_Files) {
            entry.CleanupOpenedHandles(fileToClose);
        }
    }
    
private:
    std::string m_ZipPath;
    std::shared_ptr<mz_zip_archive> m_ZipArchive;
    bool m_IsInitialized;
    mutable std::mutex m_Mutex;

    FilesList m_FileList;
    std::unordered_map<std::string, FileEntry> m_Files;
};

} // namespace vfspp

#endif // ZIPFILESYSTEM_HPP
