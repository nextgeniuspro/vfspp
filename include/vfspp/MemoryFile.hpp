#ifndef MEMORYFILE_HPP
#define MEMORYFILE_HPP

#include "IFile.h"

namespace vfspp
{

using MemoryFilePtr = std::shared_ptr<class MemoryFile>;
using MemoryFileWeakPtr = std::weak_ptr<class MemoryFile>;

class MemoryFile final : public IFile
{
    friend class MemoryFileSystem;
public:
    MemoryFile(const FileInfo& fileInfo)
        : m_FileInfo(fileInfo)
        , m_IsReadOnly(true)
        , m_IsOpened(false)
        , m_Mode(FileMode::Read)
    {
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
        return m_FileInfo;
    }
    
    /*
     * Returns file size
     */
    virtual uint64_t Size() override
    {
        if (IsOpened()) {
            return m_Data.size();
        }
        
        return 0;
    }
    
    /*
     * Check is readonly filesystem
     */
    virtual bool IsReadOnly() const override
    {
        return m_IsReadOnly;
    }
    
    /*
     * Open file for reading/writting
     */
    virtual void Open(FileMode mode) override
    {
        if (IsOpened() && m_Mode == mode) {
            Seek(0, Origin::Begin);
            return;
        }
        
        m_Mode = mode;
        m_SeekPos = 0;
        m_IsReadOnly = true;
        
        std::ios_base::openmode open_mode = static_cast<std::ios_base::openmode>(0x00);
        if ((mode & FileMode::Write) == FileMode::Write) {
            m_IsReadOnly = false;
        }
        if ((mode & FileMode::Append) == FileMode::Append) {
            m_IsReadOnly = false;
            m_SeekPos = Size() > 0 ? Size() - 1 : 0;
        }
        if ((mode & FileMode::Truncate) == FileMode::Truncate) {
            m_Data.clear();
        }
        
        m_IsOpened = true;
    }
    
    /*
     * Close file
     */
    virtual void Close() override
    {
        m_IsReadOnly = true;
        m_IsOpened = false;
        m_SeekPos = 0;
        m_Mode = FileMode::Read;
    }
    
    /*
     * Check is file ready for reading/writing
     */
    virtual bool IsOpened() const override
    {
        return m_IsOpened;
    }
    
    /*
     * Seek on a file
     */
    virtual uint64_t Seek(uint64_t offset, Origin origin) override
    {
        if (!IsOpened()) {
            return 0;
        }
        
        if (origin == Origin::Begin) {
            m_SeekPos = offset;
        } else if (origin == Origin::End) {
            m_SeekPos = Size() - offset;
        } else if (origin == Origin::Set) {
            m_SeekPos += offset;
        }
        m_SeekPos = std::min(m_SeekPos, Size() - 1);

        return Tell();
    }
    /*
     * Returns offset in file
     */
    virtual uint64_t Tell() override
    {
        return m_SeekPos;
    }
    
    /*
     * Read data from file to buffer
     */
    virtual uint64_t Read(uint8_t* buffer, uint64_t size) override
    {
        if (!IsOpened()) {
            return 0;
        }

        // Skip reading if file is not opened for reading
        if ((m_Mode & FileMode::Read) != FileMode::Read) {
            return 0;
        }
        
        uint64_t leftSize = Size() - Tell();
        uint64_t maxSize = std::min(size, leftSize);
        if (maxSize > 0) {
            memcpy(buffer, m_Data.data(), static_cast<size_t>(maxSize));
            return maxSize;
        }

        return 0;
    }
    /*
     * Write buffer data to file
     */
    virtual uint64_t Write(const uint8_t* buffer, uint64_t size) override
    {
        if (!IsOpened() || IsReadOnly()) {
            return 0;
        }
        
        // Skip writing if file is not opened for writing
        if ((m_Mode & FileMode::Write) != FileMode::Write) {
            return 0;
        }
        
        uint64_t leftSize = Size() - Tell();
        if (size > leftSize) {
            m_Data.resize((size_t)(m_Data.size() + (size - leftSize)));
        }
        memcpy(m_Data.data() + Tell(), buffer, static_cast<size_t>(size));
        
        return size;
    }
    
private:
    std::vector<uint8_t> m_Data;
    FileInfo m_FileInfo;
    bool m_IsReadOnly;
    bool m_IsOpened;
    uint64_t m_SeekPos;
    FileMode m_Mode;
};
    
} // namespace vfspp

#endif // MEMORYFILE_HPP
