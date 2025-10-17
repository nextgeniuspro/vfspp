#ifndef VFSPP_NATIVEFILE_HPP
#define VFSPP_NATIVEFILE_HPP

#include "IFile.h"
#include "ThreadingPolicy.hpp"

namespace fs = std::filesystem;

namespace vfspp
{

template <typename ThreadingPolicy>
class NativeFile;

template <typename ThreadingPolicy>
using NativeFilePtr = std::shared_ptr<NativeFile<ThreadingPolicy>>;

template <typename ThreadingPolicy>
using NativeFileWeakPtr = std::weak_ptr<NativeFile<ThreadingPolicy>>;

template <typename ThreadingPolicy>
class NativeFile final : public IFile
{
public:
    NativeFile(const FileInfo& fileInfo)
        : m_FileInfo(fileInfo)
    {
    }
    
    NativeFile(const FileInfo& fileInfo, std::fstream&& stream)
        : m_FileInfo(fileInfo)
        , m_Stream(std::move(stream))
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
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return GetFileInfoImpl();
    }
    
    /*
     * Returns file size
     */
    [[nodiscard]]
    virtual uint64_t Size() const override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return SizeImpl();
    }
    
    /*
     * Check is readonly filesystem
     */
    [[nodiscard]]
    virtual bool IsReadOnly() const override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return IsReadOnlyImpl();
    }
    
    /*
     * Open file for reading/writting
     */
    [[nodiscard]]
    virtual bool Open(FileMode mode) override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return OpenImpl(mode);
    }
    
    /*
     * Close file
     */
    virtual void Close() override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        CloseImpl();
    }
    
    /*
     * Check is file ready for reading/writing
     */
    [[nodiscard]]
    virtual bool IsOpened() const override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return IsOpenedImpl();
    }
    
    /*
     * Seek on a file
     */
    virtual uint64_t Seek(uint64_t offset, Origin origin) override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return SeekImpl(offset, origin);
    }
    /*
     * Returns offset in file
     */
    [[nodiscard]]
    virtual uint64_t Tell() const override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return TellImpl();
    }
    
    /*
     * Read data from file to buffer
     */
    virtual uint64_t Read(std::span<uint8_t> buffer) override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return ReadImpl(buffer);
    }

    /*
     * Read data from file to vector
     */
    virtual uint64_t Read(std::vector<uint8_t>& buffer, uint64_t size) override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return ReadImpl(buffer, size);
    }

    /*
     * Write buffer data to file
     */
    virtual uint64_t Write(std::span<const uint8_t> buffer) override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
        return WriteImpl(buffer);
    }
    
    /*
     * Write data from vector to file
     */
    virtual uint64_t Write(const std::vector<uint8_t>& buffer) override
    {
        auto lock = ThreadingPolicy::Lock(m_Mutex);
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
        
        std::ios_base::openmode open_mode = std::ios_base::binary;
        if (IFile::ModeHasFlag(mode, FileMode::Read)) {
            open_mode |= std::fstream::in;
        }
        if (IFile::ModeHasFlag(mode, FileMode::Write)) {
            open_mode |= std::fstream::out;
        }
        if (IFile::ModeHasFlag(mode, FileMode::Append)) {
            open_mode |= std::fstream::app;
        }
        if (IFile::ModeHasFlag(mode, FileMode::Truncate)) {
            open_mode |= std::fstream::trunc;
        }
        
        m_Stream.open(m_FileInfo.NativePath(), open_mode);
        return m_Stream.is_open();
    }

    inline void CloseImpl()
    {
        if (IsOpenedImpl()) {
            m_Stream.close();
            m_Mode = FileMode::Read;
        }
    }

    inline bool IsOpenedImpl() const
    {
        return m_Stream.is_open();
    }

    inline uint64_t SeekImpl(uint64_t offset, Origin origin)
    {
        if (!IsOpenedImpl()) {
            return 0;
        }
        
        std::ios_base::seekdir dir = std::ios_base::beg;
        if (origin == IFile::Origin::End) {
            dir = std::ios_base::end;
        } else if (origin == IFile::Origin::Set) {
            dir = std::ios_base::cur;
        }
        
        // Seek both read and write pointers to keep them synchronized
        m_Stream.seekg(offset, dir);
        m_Stream.seekp(offset, dir);
        
        if (m_Stream.fail()) {
            m_Stream.clear();
            return 0;
        }

        return TellImpl();
    }

    inline uint64_t TellImpl() const
    {
        if (!IsOpenedImpl()) {
            return 0;
        }
        
        if (IFile::ModeHasFlag(m_Mode, FileMode::Read)) {
            auto pos = m_Stream.tellg();
            return (pos != std::streampos(-1)) ? static_cast<uint64_t>(pos) : 0;
        } else if (IFile::ModeHasFlag(m_Mode, FileMode::Write)) {
            auto pos = m_Stream.tellp();
            return (pos != std::streampos(-1)) ? static_cast<uint64_t>(pos) : 0;
        }
        
        return 0;
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

        const auto currentPos = TellImpl();
        const auto fileSize = SizeImpl();
                
        if (currentPos >= fileSize) {
            return 0;
        }
        const auto bytesLeft = fileSize - currentPos;

        const auto requestedBytes = static_cast<uint64_t>(buffer.size_bytes());
        if (requestedBytes == 0) {
            return 0;
        }

        auto bytesToRead = std::min(bytesLeft, requestedBytes);
        if (bytesToRead == 0) {
            return 0;
        }

        m_Stream.read(reinterpret_cast<char*>(buffer.data()), bytesToRead);
        if (m_Stream.good() || m_Stream.eof()) {
            return static_cast<uint64_t>(m_Stream.gcount());
        }
        // Clear error flags for failed read
        m_Stream.clear();
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

        m_Stream.write(reinterpret_cast<const char*>(buffer.data()), writeSize);
        if (m_Stream.good()) {
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
    mutable std::fstream m_Stream;
    FileMode m_Mode = FileMode::Read;
    mutable std::mutex m_Mutex;
};

using MultiThreadedNativeFile = NativeFile<MultiThreadedPolicy>;
using MultiThreadedNativeFilePtr = NativeFilePtr<MultiThreadedPolicy>;
using MultiThreadedNativeFileWeakPtr = NativeFileWeakPtr<MultiThreadedPolicy>;

using SingleThreadedNativeFile = NativeFile<SingleThreadedPolicy>;
using SingleThreadedNativeFilePtr = NativeFilePtr<SingleThreadedPolicy>;
using SingleThreadedNativeFileWeakPtr = NativeFileWeakPtr<SingleThreadedPolicy>;
    
} // namespace vfspp

#endif // VFSPP_NATIVEFILE_HPP