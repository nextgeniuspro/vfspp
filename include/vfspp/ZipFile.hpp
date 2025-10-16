#ifndef VFSPP_ZIPFILE_HPP
#define VFSPP_ZIPFILE_HPP

#include "IFile.h"
#include "ThreadingPolicy.hpp"
#include "zip_file.hpp"

#include <span>

namespace vfspp
{

template <typename ThreadingPolicy>
class ZipFile;

template <typename ThreadingPolicy>
using ZipFilePtr = std::shared_ptr<ZipFile<ThreadingPolicy>>;

template <typename ThreadingPolicy>
using ZipFileWeakPtr = std::weak_ptr<ZipFile<ThreadingPolicy>>;


template <typename ThreadingPolicy>
class ZipFile final : public IFile
{
public:
    ZipFile(const FileInfo& fileInfo, uint32_t entryID, uint64_t size, std::shared_ptr<mz_zip_archive> zipArchive)
        : m_FileInfo(fileInfo)
        , m_EntryID(entryID)
        , m_Size(size)
        , m_ZipArchive(zipArchive)
    {
    }   

    ~ZipFile()
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
        return m_Size;
    }
    
    inline bool IsReadOnlyImpl() const
    {
        return true;
    }
    
    inline bool OpenImpl(FileMode mode)
    {
        if (!IFile::IsModeValid(mode)) {
            return false;
        }

        if (IsReadOnlyImpl() && IFile::ModeHasFlag(mode, FileMode::Write)) {
            return false;
        }

        if (IsOpenedImpl()) {
            SeekImpl(0, IFile::Origin::Begin);
            return true;
        }

        m_SeekPos = 0;

        std::shared_ptr<mz_zip_archive> zipArchive = m_ZipArchive.lock();
        if (!zipArchive) {
            return false;
        }
                
        return true;
    }
    
    inline void CloseImpl()
    {
        m_SeekPos = 0;
    }
    
    inline bool IsOpenedImpl() const
    {
        return !m_ZipArchive.expired();
    }
    
    inline uint64_t SeekImpl(uint64_t offset, Origin origin)
    {
        if (!IsOpenedImpl()) {
            return 0;
        }

        const auto size = SizeImpl();
        
        if (origin == IFile::Origin::Begin) {
            m_SeekPos = offset;
        } else if (origin == IFile::Origin::End) {
            m_SeekPos = (offset <= size) ? size - offset : 0;
        } else if (origin == IFile::Origin::Set) {
            m_SeekPos += offset;
        }
        m_SeekPos = std::min(m_SeekPos, size);

        return m_SeekPos;
    }
    
    inline uint64_t TellImpl() const
    {
        return m_SeekPos;
    }
    
    struct PartialExtractContext {
        size_t Offset;              // Bytes to skip
        size_t SizeToRead;          // Number of bytes we want to read
        size_t TotalRead;           // How many bytes written so far
        unsigned char* OutBuffer;   // Pointer to output buffer
    };

    // Callback used by miniz during extraction
    static size_t partialExtractCallback(void* opaque, mz_uint64 fileOffset, const void* buffer, size_t size)
    {
        PartialExtractContext* ctx = reinterpret_cast<PartialExtractContext*>(opaque);

        // If data comes before the desired offset, skip it
        if (fileOffset + size <= ctx->Offset) {
            // Entire block is before the target range, skip
            return size;
        }

        // Determine how much of this block overlaps with the desired range
        size_t startInBlock = 0;
        if (fileOffset < ctx->Offset) {
            startInBlock = ctx->Offset - fileOffset;
        }

        size_t available = size - startInBlock;

        // Stop if we've already read enough
        if (ctx->TotalRead >= ctx->SizeToRead) {
            return size;
        }

        // How much we can copy this time
        size_t remaining = ctx->SizeToRead - ctx->TotalRead;
        size_t numToCopy = (available < remaining) ? available : remaining;

        // Copy data into buffer
        std::memcpy(ctx->OutBuffer + ctx->TotalRead, static_cast<const unsigned char*>(buffer) + startInBlock, numToCopy);

        ctx->TotalRead += numToCopy;
        return size;
    }

    inline uint64_t ReadImpl(std::span<uint8_t> buffer)
    {
        if (!IsOpenedImpl()) {
            return 0;
        }

        std::shared_ptr<mz_zip_archive> zip = m_ZipArchive.lock();
        if (!zip) {
            return 0;
        }
                
        if (m_Size <= m_SeekPos) {
            return 0;
        }

        const auto requestedBytes = static_cast<uint64_t>(buffer.size_bytes());
        if (requestedBytes == 0) {
            return 0;
        }

        const auto bytesLeft = m_Size - m_SeekPos;
        auto bytesToRead = std::min(bytesLeft, requestedBytes);
        if (bytesToRead == 0) {
            return 0;
        }

        PartialExtractContext ctx{};
        ctx.Offset = m_SeekPos;
        ctx.SizeToRead = bytesToRead;
        ctx.TotalRead = 0;
        ctx.OutBuffer = static_cast<unsigned char*>(buffer.data());

        mz_bool ok = mz_zip_reader_extract_to_callback(
            zip.get(),
            m_EntryID,
            partialExtractCallback,
            &ctx,
            0  // flags
        );

        if (!ok) {
            return 0;
        }

        m_SeekPos += ctx.TotalRead;
        return ctx.TotalRead;
    }
    
    inline uint64_t WriteImpl(std::span<const uint8_t> buffer)
    {
        return 0;
    }

    inline uint64_t ReadImpl(std::vector<uint8_t>& buffer, uint64_t size)
    {
        buffer.resize(size);
        return ReadImpl(std::span<uint8_t>(buffer.data(), buffer.size()));
    }
    
    inline uint64_t WriteImpl(const std::vector<uint8_t>& buffer)
    {
        return 0;
    }

private:
    FileInfo m_FileInfo;
    uint32_t m_EntryID;
    uint64_t m_Size;
    std::weak_ptr<mz_zip_archive> m_ZipArchive;
    uint64_t m_SeekPos = 0;
    mutable std::mutex m_Mutex;
};

using MultiThreadedZipFile = ZipFile<MultiThreadedPolicy>;
using MultiThreadedZipFilePtr = ZipFilePtr<MultiThreadedPolicy>;
using MultiThreadedZipFileWeakPtr = ZipFileWeakPtr<MultiThreadedPolicy>;

using SingleThreadedZipFile = ZipFile<SingleThreadedPolicy>;
using SingleThreadedZipFilePtr = ZipFilePtr<SingleThreadedPolicy>;
using SingleThreadedZipFileWeakPtr = ZipFileWeakPtr<SingleThreadedPolicy>;
    
} // namespace vfspp

#endif // VFSPP_ZIPFILE_HPP