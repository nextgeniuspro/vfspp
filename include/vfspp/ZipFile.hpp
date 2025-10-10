#ifndef ZIPFILE_HPP
#define ZIPFILE_HPP

#include "IFile.h"
#include "zip_file.hpp"

#include <span>

namespace vfspp
{

using ZipFilePtr = std::shared_ptr<class ZipFile>;
using ZipFileWeakPtr = std::weak_ptr<class ZipFile>;


class ZipFile final : public IFile
{
public:
    ZipFile(const FileInfo& fileInfo, uint32_t entryID, uint64_t size, std::shared_ptr<mz_zip_archive> zipArchive)
        : m_FileInfo(fileInfo)
        , m_EntryID(entryID)
        , m_Size(size)
        , m_ZipArchive(zipArchive)
        , m_SeekPos(0)
    {
    }   

    ~ZipFile()
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
        return FileMode::Read;
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
        
        return m_Size;
    }
    
    inline bool IsReadOnlyST() const
    {
        return true;
    }
    
    inline bool OpenST(FileMode mode)
    {
        if (!IFile::IsModeValid(mode)) {
            return false;
        }

        if (IsReadOnlyST() && IFile::ModeHasFlag(mode, FileMode::Write)) {
            return false;
        }

        if (IsOpenedST()) {
            SeekST(0, IFile::Origin::Begin);
            return true;
        }

        m_SeekPos = 0;

        std::shared_ptr<mz_zip_archive> zipArchive = m_ZipArchive.lock();
        if (!zipArchive) {
            return false;
        }
                
        return true;
    }
    
    inline void CloseST()
    {
        m_SeekPos = 0;
    }
    
    inline bool IsOpenedST() const
    {
        return !m_ZipArchive.expired();
    }
    
    inline uint64_t SeekST(uint64_t offset, Origin origin)
    {
        if (!IsOpenedST()) {
            return 0;
        }

        const auto size = SizeST();
        
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
    
    inline uint64_t TellST()
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

    inline uint64_t ReadST(std::span<uint8_t> buffer)
    {
        if (!IsOpenedST()) {
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
            return false;
        }

        m_SeekPos += ctx.TotalRead;
        return ctx.TotalRead;
    }
    
    inline uint64_t WriteST(std::span<const uint8_t> buffer)
    {
        return 0;
    }

    inline uint64_t ReadST(std::vector<uint8_t>& buffer, uint64_t size)
    {
        buffer.resize(size);
        return ReadST(std::span<uint8_t>(buffer.data(), buffer.size()));
    }
    
    inline uint64_t WriteST(const std::vector<uint8_t>& buffer)
    {
        return 0;
    }

private:
    FileInfo m_FileInfo;
    uint32_t m_EntryID;
    uint64_t m_Size;
    std::weak_ptr<mz_zip_archive> m_ZipArchive;
    uint64_t m_SeekPos;
    mutable std::mutex m_StateMutex;
};
    
} // namespace vfspp

#endif // ZIPFILE_HPP
