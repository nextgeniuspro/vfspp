#ifndef VFSPP_NATIVEFILE_HPP
#define VFSPP_NATIVEFILE_HPP

#include "IFile.h"
#include "ThreadingPolicy.hpp"

#ifdef VFSPP_DISABLE_STD_FILESYSTEM
#include "FilesystemCompat.hpp"
namespace fs = vfspp::fs_compat;
#else
namespace fs = std::filesystem;
#endif

namespace vfspp
{

using NativeFilePtr = std::shared_ptr<class NativeFile>;
using NativeFileWeakPtr = std::weak_ptr<class NativeFile>;


class NativeFile final : public IFile
{
public:
    NativeFile(const FileInfo& fileInfo)
        : m_FileInfo(fileInfo)
    {
    }
    
    NativeFile(const FileInfo& fileInfo, FILE* stream)
        : m_FileInfo(fileInfo)
        , m_File(stream)
    {
    }

    ~NativeFile()
    {
        Close();
    }
    
    /*
     * Get file information
     */
    [[nodiscard]]
    virtual const FileInfo& GetFileInfo() const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return GetFileInfoImpl();
    }
    
    /*
     * Returns file size
     */
    [[nodiscard]]
    virtual uint64_t Size() const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return SizeImpl();
    }
    
    /*
     * Check is readonly filesystem
     */
    [[nodiscard]]
    virtual bool IsReadOnly() const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return IsReadOnlyImpl();
    }
    
    /*
     * Open file for reading/writting
     */
    [[nodiscard]]
    virtual bool Open(FileMode mode) override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return OpenImpl(mode);
    }
    
    /*
     * Close file
     */
    virtual void Close() override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        CloseImpl();
    }
    
    /*
     * Check is file ready for reading/writing
     */
    [[nodiscard]]
    virtual bool IsOpened() const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return IsOpenedImpl();
    }
    
    /*
     * Seek on a file
     */
    virtual uint64_t Seek(uint64_t offset, Origin origin) override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return SeekImpl(offset, origin);
    }
    /*
     * Returns offset in file
     */
    [[nodiscard]]
    virtual uint64_t Tell() const override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return TellImpl();
    }
    
    /*
     * Read data from file to buffer
     */
    virtual uint64_t Read(std::span<uint8_t> buffer) override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return ReadImpl(buffer);
    }

    /*
     * Read data from file to vector
     */
    virtual uint64_t Read(std::vector<uint8_t>& buffer, uint64_t size) override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return ReadImpl(buffer, size);
    }

    /*
     * Write buffer data to file
     */
    virtual uint64_t Write(std::span<const uint8_t> buffer) override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return WriteImpl(buffer);
    }
    
    /*
     * Write data from vector to file
     */
    virtual uint64_t Write(const std::vector<uint8_t>& buffer) override
    {
        [[maybe_unused]] auto lock = ThreadingPolicy::Lock(m_Mutex);
        return WriteImpl(buffer);
    }

private:
    inline const FileInfo& GetFileInfoImpl() const
    {
        return m_FileInfo;
    }

    inline uint64_t SizeImpl() const
    {
        if (!IsOpenedImpl()) {
            return 0;
        }

        std::error_code ec;
        auto size = fs::file_size(m_FileInfo.NativePath(), ec);
        if (ec) {
            return 0;
        } 
            
        return size;
    }

    inline bool IsReadOnlyImpl() const
    {
        return !IFile::ModeHasFlag(m_Mode, FileMode::Write);
    }

    inline bool OpenImpl(FileMode mode)
    {
        if (!IFile::IsModeValid(mode)) {
            return false;
        }

        if (IsOpenedImpl() && m_Mode == mode) {
            SeekImpl(0, Origin::Begin);
            return true;
        }
        
        m_Mode = mode;
        
        bool rd = IFile::ModeHasFlag(mode, FileMode::Read);
        bool wr = IFile::ModeHasFlag(mode, FileMode::Write);
        bool app = IFile::ModeHasFlag(mode, FileMode::Append);
        bool trunc = IFile::ModeHasFlag(mode, FileMode::Truncate);

        std::string modeStr;
        if (rd && !wr) {
            modeStr = "rb";
        } else if (wr && !rd) {
            modeStr = app ? "ab" : (trunc ? "wb" : "r+b");
        } else if (rd && wr) {
            modeStr = app ? "a+b" : (trunc ? "w+b" : "r+b");
        } else {
            modeStr = "rb";
        }

        // Always use stdio FILE* implementation
        m_File = std::fopen(m_FileInfo.NativePath().c_str(), modeStr.c_str());
        return (m_File != nullptr);
    }

    inline void CloseImpl()
    {
        if (IsOpenedImpl()) {
            std::fclose(m_File);
            m_File = nullptr;
            m_Mode = FileMode::Read;
        }
    }

    inline bool IsOpenedImpl() const
    {
        return m_File != nullptr;
    }

    inline uint64_t SeekImpl(uint64_t offset, Origin origin)
    {
        if (!IsOpenedImpl()) {
            return 0;
        }

        int whence = SEEK_SET;
        if (origin == IFile::Origin::End) {
            whence = SEEK_END;
        } else if (origin == IFile::Origin::Set) {
            whence = SEEK_CUR;
        }

        if (std::fseek(m_File, static_cast<long>(offset), whence) != 0) {
            return 0;
        }

        return TellImpl();
    }

    inline uint64_t TellImpl() const
    {
        if (!IsOpenedImpl()) {
            return 0;
        }

        long pos = std::ftell(m_File);
        return (pos != -1L) ? static_cast<uint64_t>(pos) : 0;
    }

    inline uint64_t ReadImpl(std::span<uint8_t> buffer)
    {
        if (!IsOpenedImpl()) {
            return 0;
        }

        // Skip reading if file is not opened for reading
        if (!IFile::ModeHasFlag(m_Mode, FileMode::Read)) {
            return 0;
        }

        size_t read = std::fread(buffer.data(), 1, static_cast<size_t>(buffer.size_bytes()), m_File);
        if (read > 0) {
            return static_cast<uint64_t>(read);
        }
        return 0;
    }

    inline uint64_t WriteImpl(std::span<const uint8_t> buffer)
    {
        if (!IsOpenedImpl()) {
            return 0;
        }
        
        // Skip writing if file is opened for reading only
        if (!IFile::ModeHasFlag(m_Mode, FileMode::Write)) {
            return 0;
        }

        const auto writeSize = buffer.size_bytes();
        if (writeSize == 0) {
            return 0;
        }

        size_t written = std::fwrite(buffer.data(), 1, static_cast<size_t>(writeSize), m_File);
        if (written == writeSize) {
            return writeSize;
        }
        return 0;
    }

    inline uint64_t ReadImpl(std::vector<uint8_t>& buffer, uint64_t size)
    {
        buffer.resize(size);
        return ReadImpl(std::span<uint8_t>(buffer.data(), buffer.size()));
    }

    inline uint64_t WriteImpl(const std::vector<uint8_t>& buffer)
    {
        return WriteImpl(std::span<const uint8_t>(buffer.data(), buffer.size()));
    }
    
private:
    FileInfo m_FileInfo;
    std::FILE* m_File = nullptr;
    FileMode m_Mode = FileMode::Read;
    mutable std::mutex m_Mutex;
};
    
} // namespace vfspp

#endif // VFSPP_NATIVEFILE_HPP