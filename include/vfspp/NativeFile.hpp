#ifndef NATIVEFILE_HPP
#define NATIVEFILE_HPP

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
        , m_Mode(FileMode::Read)
    {
    }
    
    NativeFile(const FileInfo& fileInfo, std::fstream&& stream)
        : m_FileInfo(fileInfo)
        , m_Stream(std::move(stream))
        , m_Mode(FileMode::Read)
    {
    }

    ~NativeFile()
    {
        Close();
    }
    
    /*
     * Get file information
     */
    virtual const FileInfo& GetFileInfo() const override
    {
        auto lock = ThreadingPolicy::Lock(m_StateMutex);
        return GetFileInfoST();
    }
    
    /*
     * Returns file size
     */
    virtual uint64_t Size() override
    {
        auto lock = ThreadingPolicy::Lock(m_StateMutex);
        return SizeST();
    }
    
    /*
     * Check is readonly filesystem
     */
    virtual bool IsReadOnly() const override
    {
        auto lock = ThreadingPolicy::Lock(m_StateMutex);
        return IsReadOnlyST();
    }
    
    /*
     * Open file for reading/writting
     */
    virtual bool Open(FileMode mode) override
    {
        auto lock = ThreadingPolicy::Lock(m_StateMutex);
        return OpenST(mode);
    }
    
    /*
     * Close file
     */
    virtual void Close() override
    {
        auto lock = ThreadingPolicy::Lock(m_StateMutex);
        CloseST();
    }
    
    /*
     * Check is file ready for reading/writing
     */
    virtual bool IsOpened() const override
    {
        auto lock = ThreadingPolicy::Lock(m_StateMutex);
        return IsOpenedST();
    }
    
    /*
     * Seek on a file
     */
    virtual uint64_t Seek(uint64_t offset, Origin origin) override
    {
        auto lock = ThreadingPolicy::Lock(m_StateMutex);
        return SeekST(offset, origin);
    }
    /*
     * Returns offset in file
     */
    virtual uint64_t Tell() override
    {
        auto lock = ThreadingPolicy::Lock(m_StateMutex);
        return TellST();
    }
    
    /*
     * Read data from file to buffer
     */
    virtual uint64_t Read(std::span<uint8_t> buffer) override
    {
        auto lock = ThreadingPolicy::Lock(m_StateMutex);
        return ReadST(buffer);
    }

    /*
     * Read data from file to vector
     */
    virtual uint64_t Read(std::vector<uint8_t>& buffer, uint64_t size) override
    {
        auto lock = ThreadingPolicy::Lock(m_StateMutex);
        return ReadST(buffer, size);
    }

    /*
     * Write buffer data to file
     */
    virtual uint64_t Write(std::span<const uint8_t> buffer) override
    {
        auto lock = ThreadingPolicy::Lock(m_StateMutex);
        return WriteST(buffer);
    }
    
    /*
     * Write data from vector to file
     */
    virtual uint64_t Write(const std::vector<uint8_t>& buffer) override
    {
        auto lock = ThreadingPolicy::Lock(m_StateMutex);
        return WriteST(buffer);
    }

    /*
    * Get current file mode
    */
    virtual FileMode GetMode() const override
    {
        auto lock = ThreadingPolicy::Lock(m_StateMutex);
        return m_Mode;
    }

private:
    inline const FileInfo& GetFileInfoST() const
    {
        return m_FileInfo;
    }

    inline uint64_t SizeST()
    {
        if (!IsOpenedST()) {
            return 0;
        }

        std::error_code ec;
        auto size = fs::file_size(m_FileInfo.AbsolutePath(), ec);
        if (ec) {
            return 0;
        } 
            
        return size;
    }
    
    inline bool IsReadOnlyST() const
    {
        return IFile::IsModeWriteable(m_Mode) == false;
    }

    inline bool OpenST(FileMode mode)
    {
        if (!IFile::IsModeValid(mode)) {
            return false;
        }

        if (IsOpenedST() && m_Mode == mode) {
            SeekST(0, Origin::Begin);
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
        
        m_Stream.open(GetFileInfoST().AbsolutePath(), open_mode);
        return m_Stream.is_open();
    }

    inline void CloseST()
    {
        if (IsOpenedST()) {
            m_Stream.close();
            m_Mode = FileMode::Read;
        }
    }

    inline bool IsOpenedST() const
    {
        return m_Stream.is_open();
    }
    
    inline uint64_t SeekST(uint64_t offset, Origin origin)
    {
        if (!IsOpenedST()) {
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

        return TellST();
    }

    inline uint64_t TellST()
    {
        if (!IsOpenedST()) {
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
    
    inline uint64_t ReadST(std::span<uint8_t> buffer)
    {
        if (!IsOpenedST()) {
            return 0;
        }

        // Skip reading if file is not opened for reading
        if (!IFile::ModeHasFlag(m_Mode, FileMode::Read)) {
            return 0;
        }

        const auto currentPos = TellST();
        const auto fileSize = SizeST();
                
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

    inline uint64_t WriteST(std::span<const uint8_t> buffer)
    {
        if (!IsOpenedST()) {
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

    inline uint64_t ReadST(std::vector<uint8_t>& buffer, uint64_t size)
    {
        buffer.resize(size);
        return ReadST(std::span<uint8_t>(buffer.data(), buffer.size()));
    }
    
    inline uint64_t WriteST(const std::vector<uint8_t>& buffer)
    {
    return WriteST(std::span<const uint8_t>(buffer.data(), buffer.size()));
    }
    
private:
    FileInfo m_FileInfo;
    std::fstream m_Stream;
    FileMode m_Mode;
    mutable std::mutex m_StateMutex;
};

using MultiThreadedNativeFile = NativeFile<MultiThreadedPolicy>;
using MultiThreadedNativeFilePtr = NativeFilePtr<MultiThreadedPolicy>;
using MultiThreadedNativeFileWeakPtr = NativeFileWeakPtr<MultiThreadedPolicy>;

using SingleThreadedNativeFile = NativeFile<SingleThreadedPolicy>;
using SingleThreadedNativeFilePtr = NativeFilePtr<SingleThreadedPolicy>;
using SingleThreadedNativeFileWeakPtr = NativeFileWeakPtr<SingleThreadedPolicy>;
    
} // namespace vfspp

#endif // NATIVEFILE_HPP