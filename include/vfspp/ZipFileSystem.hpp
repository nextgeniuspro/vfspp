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
    virtual EntriesList GetEntriesList(bool excludeDirectories = true) const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return GetEntriesListST(excludeDirectories);
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
        (void)virtualPath;
        return nullptr;
    }
    
    /*
     * Remove existing file on writable filesystem
     */
    virtual bool RemoveFile(const std::string& virtualPath) override
    {
        (void)virtualPath;
        return false;
    }
    
    /*
     * Copy existing file on writable filesystem
     */
    virtual bool CopyFile(const std::string& srcVirtualPath, const std::string& dstVirtualPath, bool overwrite = false) override
    {
        (void)srcVirtualPath;
        (void)dstVirtualPath;
        (void)overwrite;
        return false;
    }
    
    /*
     * Rename existing file on writable filesystem
     */
    virtual bool RenameFile(const std::string& srcVirtualPath, const std::string& dstVirtualPath) override
    {
        (void)srcVirtualPath;
        (void)dstVirtualPath;
        return false;
    }

    virtual bool CreateDirectory(const std::string& virtualPath) override
    {
        (void)virtualPath;
        return false;
    }

    virtual bool RemoveDirectory(const std::string& virtualPath, bool recursive = false) override
    {
        (void)virtualPath;
        (void)recursive;
        return false;
    }

    virtual bool RenameDirectory(const std::string& srcVirtualPath, const std::string& dstVirtualPath) override
    {
        (void)srcVirtualPath;
        (void)dstVirtualPath;
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

    [[nodiscard]]
    virtual bool IsDirectoryExists(const std::string& virtualPath) const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return IsDirectoryExistsImpl(virtualPath);
    }

    [[nodiscard]]
    virtual std::optional<EntryInfo> GetEntryInfo(const std::string& virtualPath) const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return GetEntryInfoImpl(virtualPath);
    }

private:
    static std::string ParentPath(const std::string& path)
    {
        const size_t slash = path.find_last_of('/');
        if (slash == std::string::npos) {
            return {};
        }
        if (slash == 0) {
            return "/";
        }
        return path.substr(0, slash);
    }

    struct Entry
    {
        EntryInfo Info;
        std::vector<ZipFileWeakPtr> OpenedHandles;

        uint32_t EntryID;
        uint64_t Size;

        explicit Entry(const EntryInfo& info, uint32_t entryID, uint64_t size)
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

        BuildFilelist(AliasPathImpl(), BasePathImpl(), m_ZipArchive, m_Entries);
        m_IsInitialized = true;
        return true;
    }

    inline void ShutdownImpl()
    {
        m_ZipPath = "";
        m_Entries.clear();

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

    inline EntriesList GetEntriesListST(bool excludeDirectories) const
    {
        EntriesList fileList;
        fileList.reserve(m_Entries.size());
        for (const auto& [path, entry] : m_Entries) {
            if (excludeDirectories && entry.Info.IsDirectory()) {
                continue;
            }
            fileList.push_back(entry.Info);
        }
        return fileList;
    }
    
    inline IFilePtr OpenFileImpl(const std::string& virtualPath, IFile::FileMode mode)
    {
        const EntryInfo fileInfo(AliasPathImpl(), BasePathImpl(), virtualPath);
        const auto entryIt = m_Entries.find(fileInfo.VirtualPath());
        if (entryIt == m_Entries.end() || entryIt->second.Info.IsDirectory()) {
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
        const EntryInfo fileInfo(AliasPathImpl(), BasePathImpl(), virtualPath);
        const auto it = m_Entries.find(fileInfo.VirtualPath());
        return it != m_Entries.end() && it->second.Info.IsFile();
    }

    inline bool IsDirectoryExistsImpl(const std::string& virtualPath) const
    {
        const EntryInfo directoryInfo(AliasPathImpl(), BasePathImpl(), virtualPath, EntryType::Directory);
        const auto it = m_Entries.find(directoryInfo.VirtualPath());
        return it != m_Entries.end() && it->second.Info.IsDirectory();
    }

    inline std::optional<EntryInfo> GetEntryInfoImpl(const std::string& virtualPath) const
    {
        const EntryInfo fileInfo(AliasPathImpl(), BasePathImpl(), virtualPath);
        const auto it = m_Entries.find(fileInfo.VirtualPath());
        if (it == m_Entries.end()) {
            return std::nullopt;
        }

        return it->second.Info;
    }

    void InsertMissingAncestorDirectories(const EntryInfo& entryInfo, std::unordered_map<std::string, Entry>& outEntries)
    {
        std::string currentPath = ParentPath(entryInfo.VirtualPath());
        std::vector<std::string> missingDirectories;

        while (!currentPath.empty()) {
            if (currentPath == AliasPathImpl()) {
                break;
            }

            if (outEntries.find(currentPath) != outEntries.end()) {
                currentPath = ParentPath(currentPath);
                continue;
            }

            missingDirectories.push_back(currentPath);
            currentPath = ParentPath(currentPath);
        }

        for (auto it = missingDirectories.rbegin(); it != missingDirectories.rend(); ++it) {
            EntryInfo directoryInfo(AliasPathImpl(), BasePathImpl(), *it, EntryType::Directory);
            outEntries.emplace(directoryInfo.VirtualPath(), Entry(directoryInfo, 0, 0));
        }
    }

    void BuildFilelist(const std::string& aliasPath, const std::string& basePath, std::shared_ptr<mz_zip_archive> zipArchive, std::unordered_map<std::string, Entry>& outEntries)
    {
        for (mz_uint i = 0; i < mz_zip_reader_get_num_files(zipArchive.get()); i++) {
            mz_zip_archive_file_stat file_stat;
            if (!mz_zip_reader_file_stat(zipArchive.get(), i, &file_stat)) {
                continue;
            }

            std::string filename = file_stat.m_filename;
            const bool isDirectory = !filename.empty() && filename.back() == '/';
            EntryInfo entryInfo(aliasPath, basePath, filename, isDirectory ? EntryType::Directory : EntryType::File);
            if (entryInfo.VirtualPath() == AliasPathImpl()) {
                continue;
            }

            outEntries.insert_or_assign(
                entryInfo.VirtualPath(),
                Entry(
                    entryInfo,
                    static_cast<uint32_t>(file_stat.m_file_index),
                    isDirectory ? 0 : static_cast<uint64_t>(file_stat.m_uncomp_size)
                )
            );
        }

        for (mz_uint i = 0; i < mz_zip_reader_get_num_files(zipArchive.get()); i++) {
            mz_zip_archive_file_stat file_stat;
            if (!mz_zip_reader_file_stat(zipArchive.get(), i, &file_stat)) {
                continue;
            }

            std::string filename = file_stat.m_filename;
            const bool isDirectory = !filename.empty() && filename.back() == '/';
            EntryInfo entryInfo(aliasPath, basePath, filename, isDirectory ? EntryType::Directory : EntryType::File);
            if (entryInfo.VirtualPath() == AliasPathImpl()) {
                continue;
            }

            InsertMissingAncestorDirectories(entryInfo, outEntries);
        }
    }

    inline void CloseFileAndCleanupOpenedHandles(IFilePtr fileToClose = nullptr)
    {
        if (fileToClose) {
            const auto absolutePath = fileToClose->GetEntryInfo().VirtualPath();
            
            const auto it = m_Entries.find(absolutePath);
            if (it == m_Entries.end()) {
                return;
            }
            
            fileToClose->Close();
        }

        for (auto& [path, entry] : m_Entries) {
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

    std::unordered_map<std::string, Entry> m_Entries;
};

} // namespace vfspp

#endif // VFSPP_ZIPFILESYSTEM_HPP
