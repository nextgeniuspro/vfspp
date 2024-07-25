#ifndef ZIPFILE_HPP
#define ZIPFILE_HPP

#include "IFile.h"
#include "zip_file.hpp"

namespace vfspp
{

using ZipFilePtr = std::shared_ptr<class ZipFile>;
using ZipFileWeakPtr = std::weak_ptr<class ZipFile>;


class ZipFile final : public IFile
{
public:
    ZipFile(const FileInfo& fileInfo, uint32_t entryID, uint64_t size, std::weak_ptr<mz_zip_archive> zipArchive)
        : m_FileInfo(fileInfo)
        , m_EntryID(entryID)
        , m_Size(size)
        , m_ZipArchive(zipArchive)
        , m_IsOpened(false)
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
        return m_FileInfo;
    }
    
    /*
     * Returns file size
     */
    virtual uint64_t Size() override
    {
        if (IsOpened()) {
            return m_Size;
        }
        
        return 0;
    }
    
    /*
     * Check is readonly filesystem
     */
    virtual bool IsReadOnly() const override
    {
        return true;
    }
    
    /*
     * Open file for reading/writting
     */
    virtual void Open(FileMode mode) override
    {
        bool requestWrite = ((mode & IFile::FileMode::Write) == IFile::FileMode::Write);
        requestWrite |= ((mode & IFile::FileMode::Append) == IFile::FileMode::Append);
        requestWrite |= ((mode & IFile::FileMode::Truncate) == IFile::FileMode::Truncate);

        if (IsReadOnly() && requestWrite) {
            return;
        }

        if (IsOpened()) {
            Seek(0, IFile::Origin::Begin);
            return;
        }

        std::shared_ptr<mz_zip_archive> zipArchive = m_ZipArchive.lock();
        if (!zipArchive) {
            return;
        }
        
        m_SeekPos = 0;
        
        m_Data.resize(m_Size);
        m_IsOpened = mz_zip_reader_extract_to_mem_no_alloc(zipArchive.get(), m_EntryID, m_Data.data(), static_cast<size_t>(m_Size), 0, 0, 0);
    }
    
    /*
     * Close file
     */
    virtual void Close() override
    {
        m_IsOpened = false;
        m_SeekPos = 0;

        m_Data.clear();
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
        
        if (origin == IFile::Origin::Begin) {
            m_SeekPos = offset;
        } else if (origin == IFile::Origin::End) {
            m_SeekPos = Size() - offset;
        } else if (origin == IFile::Origin::Set) {
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
        return 0;
    }
    
private:
    FileInfo m_FileInfo;
    uint32_t m_EntryID;
    uint64_t m_Size;
    std::weak_ptr<mz_zip_archive> m_ZipArchive;
    std::vector<uint8_t> m_Data;
    bool m_IsOpened;
    uint64_t m_SeekPos;
};
    
} // namespace vfspp

#endif // ZIPFILE_HPP
