#ifndef VFSPP_MEMORYFILE_HPP
#define VFSPP_MEMORYFILE_HPP

#include "IFile.h"
#include "ThreadingPolicy.hpp"

namespace vfspp
{

using MemoryFileObjectPtr = std::shared_ptr<class MemoryFileObject>;
using MemoryFileObjectWeakPtr = std::weak_ptr<class MemoryFileObject>;

using MemoryFilePtr = std::shared_ptr<class MemoryFile>;
using MemoryFileWeakPtr = std::weak_ptr<class MemoryFile>;

class MemoryFileObject
{
    friend class MemoryFile;
    friend class MemoryFileSystem;

    using DataPtr = std::shared_ptr<std::vector<uint8_t>>;

public:
    MemoryFileObject() 
    {
    }

    MemoryFileObject(const MemoryFileObject& other)
    {
        CopyFrom(other);
    }

    MemoryFileObject& operator=(const MemoryFileObject& other)
    {
        if (this != &other) {
            CopyFrom(other);
        }
        return *this;
    }

    // Read access
    [[nodiscard]]
    DataPtr GetData() const noexcept 
    {
        return std::atomic_load(&m_Data);
    }

    // Write access (copy-on-write)
    [[nodiscard]]
    DataPtr GetWritableData() 
    {
        auto old = std::atomic_load(&m_Data);

        if (!old || old.use_count() == 1) {
            return old;
        }

        // Copy when shared with someone else
        auto copy = std::make_shared<std::vector<uint8_t>>(*old);
        std::atomic_store(&m_Data, copy);
        return copy;
    }

    void Reset() noexcept 
    {
        std::atomic_store(&m_Data, std::make_shared<std::vector<uint8_t>>());
    }

    void CopyFrom(const MemoryFileObject& other)
    {
        auto otherData = other.GetData();
        if (!otherData) {
            Reset();
            return;
        }

        auto copy = std::make_shared<std::vector<uint8_t>>(*otherData);
        std::atomic_store(&m_Data, std::move(copy));
    }

private:
    DataPtr m_Data = std::make_shared<std::vector<uint8_t>>();
};



class MemoryFile final : public IFile
{
    friend class MemoryFileSystem;

public:
    MemoryFile(const FileInfo& fileInfo, MemoryFileObjectPtr object)
        : m_Object(std::move(object))
        , m_FileInfo(fileInfo)
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
    inline MemoryFileObject& Object() const
    {
        assert(m_Object);
        return *m_Object;
    }

    inline const FileInfo& GetFileInfoImpl() const
    {
        return m_FileInfo;
    }
    
    inline uint64_t SizeImpl() const
    {
        if (!IsOpenedImpl()) {
            return 0;
        }
        
        auto data = m_Object->GetData();
        if (!data) {
            return 0;
        }

        return data->size();
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
        m_SeekPos = 0;
        
        if (IFile::ModeHasFlag(mode, FileMode::Append)) {
            const auto size = SizeImpl();
            m_SeekPos = size > 0 ? size - 1 : 0;
        }
        if (IFile::ModeHasFlag(mode, FileMode::Truncate)) {
            m_Object->Reset();
        }
        
        m_IsOpened = true;
        return true;
    }

    inline void CloseImpl()
    {
        m_IsOpened = false;
        m_SeekPos = 0;
        m_Mode = FileMode::Read;
    }

    inline bool IsOpenedImpl() const
    {
        return m_IsOpened;
    }

    inline uint64_t SeekImpl(uint64_t offset, Origin origin)
    {
        if (!IsOpenedImpl()) {
            return 0;
        }

        const auto size = SizeImpl();
        
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

    inline uint64_t TellImpl() const
    {
        return m_SeekPos;
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

    inline uint64_t WriteImpl(std::span<const uint8_t> buffer)
    {
        if (!IsOpenedImpl()) {
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
    MemoryFileObjectPtr m_Object;
    FileInfo m_FileInfo;
    bool m_IsOpened = false;
    uint64_t m_SeekPos = 0;
    FileMode m_Mode = FileMode::Read;
    mutable std::mutex m_Mutex;
};

} // namespace vfspp

#endif // VFSPP_MEMORYFILE_HPP
