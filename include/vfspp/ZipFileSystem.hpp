#ifndef VFSPP_ZIPFILESYSTEM_HPP
#define VFSPP_ZIPFILESYSTEM_HPP

#include "IFileSystem.h"
#include "Global.h"
#include "ThreadingPolicy.hpp"
#include "ZipFile.hpp"
#include "zip_file.hpp"

#ifdef VFSPP_DISABLE_STD_FILESYSTEM
#include "FilesystemCompat.hpp"
namespace fs = vfspp::fs_compat;
#else
namespace fs = std::filesystem;
#endif

namespace vfspp
{

using ZipFileSystemPtr = std::shared_ptr<class ZipFileSystem>;
using ZipFileSystemWeakPtr = std::weak_ptr<class ZipFileSystem>;


class ZipFileSystem final : public IFileSystem
{
public:
    ZipFileSystem(const std::string& aliasPath, const std::string& zipPath)
        : m_AliasPath(aliasPath)
        , m_ZipPath(zipPath)
    {
    }

    ~ZipFileSystem()
    {
        Shutdown();
    }
    
    /*
     * Initialize filesystem, call this method as soon as possible
     */
    [[nodiscard]]
    virtual bool Initialize() override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return InitializeImpl();
    }

    /*
     * Shutdown filesystem
     */
    virtual void Shutdown() override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        ShutdownImpl();
    }
    
    /*
     * Check if filesystem is initialized
     */
    [[nodiscard]]
    virtual bool IsInitialized() const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return IsInitializedImpl();
    }
    
    /*
     * Get base path
     */
    [[nodiscard]]
    virtual const std::string& BasePath() const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return BasePathImpl();
    }

    /*
    * Get mounted path
    */
    [[nodiscard]]
    virtual const std::string& VirtualPath() const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return AliasPathImpl();
    }
    
    /*
     * Retrieve file list according filter
     */
    [[nodiscard]]
    virtual FilesList GetFilesList() const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return GetFilesListST();
    }
    
    /*
     * Check is readonly filesystem
     */
    [[nodiscard]]
    virtual bool IsReadOnly() const override
    {
        return true;
    }
    
    /*
     * Open existing file for reading, if not exists returns null for readonly filesystem. 
     * If file not exists and filesystem is writable then create new file
     */
    virtual IFilePtr OpenFile(const std::string& virtualPath, IFile::FileMode mode) override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return OpenFileImpl(virtualPath, mode);
    }
    
    /*
     * Create file on writeable filesystem. Returns true if file created successfully
     */
    virtual IFilePtr CreateFile(const std::string& virtualPath) override
    {
        return nullptr;
    }
    
    /*
     * Remove existing file on writable filesystem
     */
    virtual bool RemoveFile(const std::string& virtualPath) override
    {
        return false;
    }
    
    /*
     * Copy existing file on writable filesystem
     */
    virtual bool CopyFile(const std::string& srcVirtualPath, const std::string& dstVirtualPath, bool overwrite = false) override
    {
        return false;
    }
    
    /*
     * Rename existing file on writable filesystem
     */
    virtual bool RenameFile(const std::string& srcVirtualPath, const std::string& dstVirtualPath) override
    {
        return false;
    }
    /*
    * Close file 
    */
    virtual void CloseFile(IFilePtr file) override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        CloseFileImpl(file);
    }
    /*
     * Check if file exists on filesystem
     */
    [[nodiscard]]
    virtual bool IsFileExists(const std::string& virtualPath) const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return IsFileExistsImpl(virtualPath);
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

    inline bool InitializeImpl()
    {
        if (m_IsInitialized) {
            return true;
        }

        if (!fs::is_regular_file(m_ZipPath)) {
            return false;
        }

        m_ZipArchive = std::make_shared<mz_zip_archive>();

        mz_bool status = mz_zip_reader_init_file(m_ZipArchive.get(), m_ZipPath.c_str(), 0);
        if (!status) {
            return false;
        }

        BuildFilelist(AliasPathImpl(), BasePathImpl(), m_ZipArchive, m_Files);
        m_IsInitialized = true;
        return true;
    }

    inline void ShutdownImpl()
    {
        m_ZipPath = "";
        m_Files.clear();

        // close zip archive
        if (m_ZipArchive) {
            mz_zip_reader_end(m_ZipArchive.get());
            m_ZipArchive = nullptr;
        }

        m_IsInitialized = false;
    }
    
    inline bool IsInitializedImpl() const
    {
        return m_IsInitialized;
    }

    inline const std::string& BasePathImpl() const
    {
        return m_BasePath; // Empty for zip filesystem
    }

    inline const std::string& AliasPathImpl() const
    {
        return m_AliasPath;
    }

    inline FilesList GetFilesListST() const
    {
        FilesList fileList;
        fileList.reserve(m_Files.size());
        for (const auto& [path, entry] : m_Files) {
            fileList.push_back(entry.Info);
        }
        return fileList;
    }
    
    inline IFilePtr OpenFileImpl(const std::string& virtualPath, IFile::FileMode mode)
    {
        const auto entryIt = m_Files.find(virtualPath);
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

    inline void CloseFileImpl(IFilePtr file)
    {
        CloseFileAndCleanupOpenedHandles(file);
    }

    inline bool IsFileExistsImpl(const std::string& virtualPath) const
    {
        return m_Files.find(virtualPath) != m_Files.end();
    }

    void BuildFilelist(const std::string& aliasPath, const std::string& basePath, std::shared_ptr<mz_zip_archive> zipArchive, std::unordered_map<std::string, FileEntry>& outFiles)
    {
        for (mz_uint i = 0; i < mz_zip_reader_get_num_files(zipArchive.get()); i++) {
            mz_zip_archive_file_stat file_stat;
            if (!mz_zip_reader_file_stat(zipArchive.get(), i, &file_stat)) {
                // TODO: log error
                continue;
            }

            // Skip directories (entries ending with '/')
            std::string filename = file_stat.m_filename;
            if (!filename.empty() && filename.back() == '/') {
                continue;
            }

            FileInfo fileInfo(aliasPath, basePath, filename);
            outFiles.emplace(
                fileInfo.VirtualPath(),
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
            const auto absolutePath = fileToClose->GetFileInfo().VirtualPath();
            
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
    std::string m_AliasPath;
    std::string m_BasePath;
    std::string m_ZipPath;
    std::shared_ptr<mz_zip_archive> m_ZipArchive = nullptr;
    bool m_IsInitialized = false;
    mutable std::mutex m_Mutex;

    std::unordered_map<std::string, FileEntry> m_Files;
};

} // namespace vfspp

#endif // VFSPP_ZIPFILESYSTEM_HPP
