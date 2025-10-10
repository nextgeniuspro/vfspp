#ifndef MEMORYFILE_HPP
#define MEMORYFILE_HPP

#include "IFile.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <limits>
#include <memory>
#include <span>
#include <vector>


namespace vfspp
{

class MemoryFileObject;

using MemoryFileObjectPtr = std::shared_ptr<MemoryFileObject>;
using MemoryFileObjectWeakPtr = std::weak_ptr<MemoryFileObject>;
using MemoryFilePtr = std::shared_ptr<class MemoryFile>;
using MemoryFileWeakPtr = std::weak_ptr<class MemoryFile>;

class MemoryFileObject
{
    friend class MemoryFile;
    friend class MemoryFileSystem;

    using DataPtr = std::shared_ptr<std::vector<uint8_t>>;

public:
    MemoryFileObject() 
        : m_Data(std::make_shared<std::vector<uint8_t>>())
    {
    }

    // Read access
    DataPtr GetData() const noexcept {
        return std::atomic_load(&m_Data);
    }

    // Write access (copy-on-write)
    DataPtr GetWritableData() {
        auto old = std::atomic_load(&m_Data);

        if (!old || old.use_count() == 1) {
            return old; // already unique or null
        }

        // Copy when shared with someone else
        auto copy = std::make_shared<std::vector<uint8_t>>(*old);
        std::atomic_store(&m_Data, copy);
        return copy;
    }

    void Reset() noexcept {
        std::atomic_store(&m_Data, std::make_shared<std::vector<uint8_t>>());
    }

private:
    DataPtr m_Data;
};

class MemoryFile final : public IFile
{
    friend class MemoryFileSystem;

public:
    MemoryFile(const FileInfo& fileInfo, MemoryFileObjectPtr object)
        : m_Object(std::move(object))
        , m_FileInfo(fileInfo)
        , m_IsOpened(false)
        , m_SeekPos(0)
        , m_Mode(FileMode::Read)
    {
        if (!m_Object) {
            m_Object = std::make_shared<MemoryFileObject>();
        }
    }

    ~MemoryFile()
    {
        Close();
    }
    
    /*
     * Get file information
     */
    virtual const FileInfo& GetFileInfo() const override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> stateLock(m_StateMutex);
            return GetFileInfoST();
        } else {
            return GetFileInfoST();
        }
    }
    
    /*
     * Returns file size
     */
    virtual uint64_t Size() override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> stateLock(m_StateMutex);
            return SizeST();
        } else {
            return SizeST();
        }
    }
    
    /*
     * Check is readonly filesystem
     */
    virtual bool IsReadOnly() const override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> stateLock(m_StateMutex);
            return IsReadOnlyST();
        } else {
            return IsReadOnlyST();
        }
    }
    
    /*
     * Open file for reading/writting
     */
    virtual bool Open(FileMode mode) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> stateLock(m_StateMutex);
            return OpenST(mode);
        } else {
            return OpenST(mode);
        }
    }
    
    /*
     * Close file
     */
    virtual void Close() override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> stateLock(m_StateMutex);
            CloseST();
        } else {
            CloseST();
        }
    }
    
    /*
     * Check is file ready for reading/writing
     */
    virtual bool IsOpened() const override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> stateLock(m_StateMutex);
            return IsOpenedST();
        } else {
            return IsOpenedST();
        }
    }
    
    /*
     * Seek on a file
     */
    virtual uint64_t Seek(uint64_t offset, Origin origin) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> stateLock(m_StateMutex);
            return SeekST(offset, origin);
        } else {
            return SeekST(offset, origin);
        }
    }
    /*
     * Returns offset in file
     */
    virtual uint64_t Tell() override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> stateLock(m_StateMutex);
            return TellST();
        } else {
            return TellST();
        }
    }
    
    /*
     * Read data from file to buffer
     */
    virtual uint64_t Read(std::span<uint8_t> buffer) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> stateLock(m_StateMutex);
            return ReadST(buffer);
        } else {
            return ReadST(buffer);
        }
    }

    /*
     * Read data from file to vector
     */
    virtual uint64_t Read(std::vector<uint8_t>& buffer, uint64_t size) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> stateLock(m_StateMutex);
            return ReadST(buffer, size);
        } else {
            return ReadST(buffer, size);
        }
    }

    /*
     * Write buffer data to file
     */
    virtual uint64_t Write(std::span<const uint8_t> buffer) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> stateLock(m_StateMutex);
            return WriteST(buffer);
        } else {
            return WriteST(buffer);
        }
    }
    
    /*
     * Write data from vector to file
     */
    virtual uint64_t Write(const std::vector<uint8_t>& buffer) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> stateLock(m_StateMutex);
            return WriteST(buffer);
        } else {
            return WriteST(buffer);
        }
    }

    /*
    * Get current file mode
    */
    virtual FileMode GetMode() const override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> stateLock(m_StateMutex);
            return m_Mode;
        } else {
            return m_Mode;
        }
    }

private:
    inline MemoryFileObject& Object() const
    {
        assert(m_Object);
        return *m_Object;
    }

    inline const FileInfo& GetFileInfoST() const
    {
        return m_FileInfo;
    }
    
    inline uint64_t SizeST()
    {
        if (!IsOpenedST()) {
            return 0;
        }
        
        auto data = m_Object->GetData();
        if (!data) {
            return 0;
        }

        return data->size();
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
        m_SeekPos = 0;
        
        if (IFile::ModeHasFlag(mode, FileMode::Append)) {
            const auto size = SizeST();
            m_SeekPos = size > 0 ? size - 1 : 0;
        }
        if (IFile::ModeHasFlag(mode, FileMode::Truncate)) {
            m_Object->Reset();
        }
        
        m_IsOpened = true;
        return true;
    }

    inline void CloseST()
    {
        m_IsOpened = false;
        m_SeekPos = 0;
        m_Mode = FileMode::Read;
    }

    inline bool IsOpenedST() const
    {
        return m_IsOpened;
    }

    inline uint64_t SeekST(uint64_t offset, Origin origin)
    {
        if (!IsOpenedST()) {
            return 0;
        }

        const auto size = SizeST();
        
        if (origin == Origin::Begin) {
            m_SeekPos = offset;
        } else if (origin == Origin::End) {
            m_SeekPos = (offset < size) ? size - offset : 0;
        } else if (origin == Origin::Set) {
            m_SeekPos += offset;
        }
        m_SeekPos = std::min(m_SeekPos, size);

        return m_SeekPos;
    }

    inline uint64_t TellST()
    {
        return m_SeekPos;
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

        auto data = m_Object->GetData();
        if (!data) {
            return 0;
        }

        const auto availableBytes = data->size();
        if (availableBytes <= m_SeekPos) {
            return 0;
        }

        const auto requestedBytes = static_cast<uint64_t>(buffer.size_bytes());
        if (requestedBytes == 0) {
            return 0;
        }

        const auto bytesLeft = availableBytes - m_SeekPos;
        auto bytesToRead = std::min(bytesLeft, requestedBytes);
        if (bytesToRead == 0) {
            return 0;
        }

        std::memcpy(buffer.data(), data->data() + m_SeekPos, static_cast<std::size_t>(bytesToRead));
        m_SeekPos += bytesToRead;
        return bytesToRead;
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

        // Copy on write
        auto data = m_Object->GetWritableData();
        if (!data) {
            return 0;
        }

        const auto writeSize = buffer.size_bytes();
        if (writeSize == 0) {
            return 0;
        }

        if (m_SeekPos + writeSize > data->size()) {
            data->resize(m_SeekPos + writeSize);
        }
        
        std::memcpy(data->data() + m_SeekPos, buffer.data(), writeSize);

        m_SeekPos += writeSize;
        return writeSize;
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
    MemoryFileObjectPtr m_Object;
    FileInfo m_FileInfo;
    bool m_IsOpened;
    uint64_t m_SeekPos;
    FileMode m_Mode;
    mutable std::mutex m_StateMutex;
};
    
} // namespace vfspp

#endif // MEMORYFILE_HPP
