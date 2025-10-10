#ifndef NATIVEFILESYSTEM_HPP
#define NATIVEFILESYSTEM_HPP

#include "IFileSystem.h"
#include "Global.h"
#include "ThreadingPolicy.hpp"
#include "NativeFile.hpp"

namespace fs = std::filesystem;

namespace vfspp
{

template <typename ThreadingPolicy>
class NativeFileSystem;

template <typename ThreadingPolicy>
using NativeFileSystemPtr = std::shared_ptr<NativeFileSystem<ThreadingPolicy>>;

template <typename ThreadingPolicy>
using NativeFileSystemWeakPtr = std::weak_ptr<NativeFileSystem<ThreadingPolicy>>;

template <typename ThreadingPolicy>
class NativeFileSystem final : public IFileSystem
{
public:
    NativeFileSystem(const std::string& basePath)
        : m_BasePath(basePath)
        , m_IsInitialized(false)
    {
    }

    ~NativeFileSystem()
    {
        Shutdown();
    }
    
    /*
     * Initialize filesystem, call this method as soon as possible
     */
    virtual void Initialize() override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        InitializeST();
    }

    /*
     * Shutdown filesystem
     */
    virtual void Shutdown() override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        ShutdownST();
    }
    
    /*
     * Check if filesystem is initialized
     */
    virtual bool IsInitialized() const override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return IsInitializedST();
    }
    
    /*
     * Get base path
     */
    virtual const std::string& BasePath() const override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return BasePathST();
    }
    
    /*
     * Retrieve file list according filter
     */
    virtual const FilesList& FileList() const override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return FileListST();
    }
    
    /*
     * Check is readonly filesystem
     */
    virtual bool IsReadOnly() const override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return IsReadOnlyST();
    }
    
    /*
     * Open existing file for reading, if not exists returns null for readonly filesystem. 
     * If file not exists and filesystem is writable then create new file
     */
    virtual IFilePtr OpenFile(const FileInfo& filePath, IFile::FileMode mode) override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return OpenFileST(filePath, mode);
    }

    /*
     * Close file
     */
    virtual void CloseFile(IFilePtr file) override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        CloseFileST(file);
    }
    
    /*
     * Create file on writeable filesystem. Returns true if file created successfully
     */
    virtual bool CreateFile(const FileInfo& filePath) override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return CreateFileST(filePath);
    }
    
    /*
     * Remove existing file on writable filesystem
     */
    virtual bool RemoveFile(const FileInfo& filePath) override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return RemoveFileST(filePath);
    }
    
    /*
     * Copy existing file on writable filesystem
     */
    virtual bool CopyFile(const FileInfo& src, const FileInfo& dest, bool overwrite = false) override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return CopyFileST(src, dest, overwrite);
    }
    
    /*
     * Rename existing file on writable filesystem
     */
    virtual bool RenameFile(const FileInfo& srcPath, const FileInfo& dstPath) override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return RenameFileST(srcPath, dstPath);
    }

    /*
     * Check if file exists on filesystem
     */
    virtual bool IsFileExists(const FileInfo& filePath) const override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return IsFileExistsST(filePath);
    }

    /*
     * Check is file
     */
    virtual bool IsFile(const FileInfo& filePath) const override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return IFileSystem::IsFile(filePath, m_FileList);
    }
    
    /*
     * Check is dir
     */
    virtual bool IsDir(const FileInfo& dirPath) const override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return IFileSystem::IsDir(dirPath, m_FileList);
    }

private:
    struct FileEntry
    {
        FileInfo Info;
        std::vector<NativeFileWeakPtr<ThreadingPolicy>> OpenedHandles;

        explicit FileEntry(const FileInfo& info)
            : Info(info)
        {
        }

        void CleanupOpenedHandles(IFilePtr fileToExclude = nullptr)
        {
            OpenedHandles.erase(std::remove_if(OpenedHandles.begin(), OpenedHandles.end(), [&](const NativeFileWeakPtr<ThreadingPolicy>& weak) {
                return weak.expired() || weak.lock() == fileToExclude;
            }), OpenedHandles.end());
        }
    };

    inline void InitializeST()
    {
        if (m_IsInitialized) {
            return;
        }

        if (!fs::is_directory(m_BasePath)) {
            return;
        }

        if (!fs::exists(m_BasePath)) { // TODO: create if not exists flag
            return;
        }

        BuildFilelist(m_BasePath, m_FileList, m_Files);
        m_IsInitialized = true;
    }

    inline void ShutdownST()
    {
        m_BasePath = "";
        m_Files.clear();
        m_FileList.clear();

        m_IsInitialized = false;
    }
    
    inline bool IsInitializedST() const
    {
        return m_IsInitialized;
    }
    
    inline const std::string& BasePathST() const
    {
        return m_BasePath;
    }

    inline const FilesList& FileListST() const
    {
        return m_FileList;
    }
    
    inline bool IsReadOnlyST() const
    {
        if (!IsInitializedST()) {
            return true;
        }
        
        auto perms = fs::status(m_BasePath).permissions();
        return (perms & fs::perms::owner_write) == fs::perms::none;
    }
    
    inline IFilePtr OpenFileST(const FileInfo& filePath, IFile::FileMode mode)
    {
        const bool requestWrite = IFile::ModeHasFlag(mode, IFile::FileMode::Write);
        if (IsReadOnlyST() && requestWrite) {
            return nullptr;
        }

        const auto it = m_Files.find(filePath.AbsolutePath());
        if (it == m_Files.end() && requestWrite) {
            m_Files.emplace(filePath.AbsolutePath(), filePath);
            m_FileList.push_back(filePath);   
        }

        const auto entryIt = m_Files.find(filePath.AbsolutePath());
        if (entryIt == m_Files.end()) {
            return nullptr;
        }
        auto& entry = entryIt->second;

        NativeFilePtr<ThreadingPolicy> file = std::make_shared<NativeFile<ThreadingPolicy>>(entry.Info);
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
    
    inline bool CreateFileST(const FileInfo& filePath)
    {
        IFilePtr file = OpenFileST(filePath, IFile::FileMode::Write | IFile::FileMode::Truncate);
        CloseFileAndCleanupOpenedHandles(file);
        return file != nullptr;
    }
    
    inline bool RemoveFileST(const FileInfo& filePath)
    {
        if (IsReadOnlyST()) {
            return false;
        }

        auto absolutePath = filePath.AbsolutePath();
        
        auto it = m_Files.find(absolutePath);
        if (it == m_Files.end()) {
            return false;
        }

        CloseFileAndCleanupOpenedHandles();

        m_Files.erase(it);
        m_FileList.erase(std::remove_if(m_FileList.begin(), m_FileList.end(), [&](const FileInfo& f) {
            return f.AbsolutePath() == absolutePath;
        }), m_FileList.end());
        
        return fs::remove(absolutePath);
    }
    
    inline bool CopyFileST(const FileInfo& srcPath, const FileInfo& dstPath, bool overwrite = false)
    {
        if (IsReadOnlyST()) {
            return false;
        }
        
        if (!IsFileExistsST(srcPath)) {
            return false;
        }

        auto option = overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::skip_existing;       

        const bool ok = fs::copy_file(srcPath.AbsolutePath(), dstPath.AbsolutePath(), option);
        if (!ok) {
            return false;
        }

        // Remove existing dest entry if any
        const auto destIt = m_Files.find(dstPath.AbsolutePath());
        if (destIt != m_Files.end()) {
            m_Files.erase(destIt);
            m_FileList.erase(std::remove_if(m_FileList.begin(), m_FileList.end(), [&](const FileInfo& f) {
                return f.AbsolutePath() == dstPath.AbsolutePath();
            }), m_FileList.end());
        }

        // Add new entry
    const auto [destEntryIt, inserted] = m_Files.insert_or_assign(dstPath.AbsolutePath(), FileEntry(dstPath));
        if (inserted) {
            m_FileList.push_back(dstPath);
        }
        
        return true;
    }
    
    bool RenameFileST(const FileInfo& srcPath, const FileInfo& dstPath)
    {
        if (IsReadOnlyST()) {
            return false;
        }
        
        if (!IsFileExistsST(srcPath)) {
            return false;
        }
        
        std::error_code ec;
        fs::rename(srcPath.AbsolutePath(), dstPath.AbsolutePath(), ec);
        if (ec) {
            return false;
        }

        // Remove existing src entry
        const auto srcIt = m_Files.find(srcPath.AbsolutePath());
        if (srcIt != m_Files.end()) {
            m_Files.erase(srcIt);
            m_FileList.erase(std::remove_if(m_FileList.begin(), m_FileList.end(), [&](const FileInfo& f) {
                return f.AbsolutePath() == srcPath.AbsolutePath();
            }), m_FileList.end());
        }

        // Remove existing dest entry if any
        const auto destIt = m_Files.find(dstPath.AbsolutePath());
        if (destIt != m_Files.end()) {
            m_Files.erase(destIt);
            m_FileList.erase(std::remove_if(m_FileList.begin(), m_FileList.end(), [&](const FileInfo& f) {
                return f.AbsolutePath() == dstPath.AbsolutePath();
            }), m_FileList.end());
        }

        // Add new entry
    const auto [destEntryIt, inserted] = m_Files.insert_or_assign(dstPath.AbsolutePath(), FileEntry(dstPath));
        if (inserted) {
            m_FileList.push_back(dstPath);
        }
    }

    inline bool IsFileExistsST(const FileInfo& filePath) const
    {
        const auto fileIt = m_Files.find(filePath.AbsolutePath());
        return fileIt != m_Files.end() && fs::exists(fileIt->second.Info.AbsolutePath());
    }

    void BuildFilelist(std::string basePath, FilesList& outFileList, std::unordered_map<std::string, FileEntry>& outFiles)
    {
        for (const auto& entry : fs::directory_iterator(basePath)) {
            auto filename = entry.path().filename().string();
            if (fs::is_directory(entry.status())) {
                BuildFilelist(entry.path().string(), outFileList, outFiles);
            } else if (fs::is_regular_file(entry.status())) {
                FileInfo fileInfo(basePath, filename, false);
                
                outFileList.push_back(fileInfo);
                outFiles.emplace(fileInfo.AbsolutePath(), fileInfo);
            }
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
    std::string m_BasePath;
    bool m_IsInitialized;
    mutable std::mutex m_Mutex;

    std::unordered_map<std::string, FileEntry> m_Files;
    FilesList m_FileList;
};

using MultiThreadedNativeFileSystem = NativeFileSystem<MultiThreadedPolicy>;
using MultiThreadedNativeFileSystemPtr = NativeFileSystemPtr<MultiThreadedPolicy>;
using MultiThreadedNativeFileSystemWeakPtr = NativeFileSystemWeakPtr<MultiThreadedPolicy>;

using SingleThreadedNativeFileSystem = NativeFileSystem<SingleThreadedPolicy>;
using SingleThreadedNativeFileSystemPtr = NativeFileSystemPtr<SingleThreadedPolicy>;
using SingleThreadedNativeFileSystemWeakPtr = NativeFileSystemWeakPtr<SingleThreadedPolicy>;

} // namespace vfspp

#endif // NATIVEFILESYSTEM_HPP
