#ifndef NATIVEFILE_H
#define NATIVEFILE_H

#include "IFile.h"

namespace vfspp
{

using NativeFilePtr = std::shared_ptr<class NativeFile>;
using NativeFileWeakPtr = std::weak_ptr<class NativeFile>;

class NativeFile final : public IFile
{
public:
    NativeFile(const FileInfo& fileInfo)
        : m_FileInfo(fileInfo)
        , m_IsReadOnly(true)
        , m_Mode(0)
    {
    }
    
    NativeFile(const FileInfo& fileInfo, std::fstream&& stream)
        : m_FileInfo(fileInfo)
        , m_Stream(std::move(stream))
        , m_IsReadOnly(true)
        , m_Mode(0)
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
        return m_FileInfo;
    }
    
    /*
     * Returns file size
     */
    virtual uint64_t Size() override
    {
        if (IsOpened()) {
            uint64_t curPos = Tell();
            Seek(0, IFile::Origin::End);
            uint64_t size = Tell();
            Seek(curPos, IFile::Origin::Begin);
            
            return size;
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
    virtual void Open(int mode) override
    {
        if (IsOpened() && m_Mode == mode) {
            Seek(0, IFile::Origin::Begin);
            return;
        }
        
        m_Mode = mode;
        m_IsReadOnly = true;
        
        std::ios_base::openmode open_mode = (std::ios_base::openmode)0x00;
        if (mode & static_cast<int>(IFile::FileMode::Read)) {
            open_mode |= std::fstream::in;
        }
        if (mode & static_cast<int>(IFile::FileMode::Write)) {
            m_IsReadOnly = false;
            open_mode |= std::fstream::out;
        }
        if (mode & static_cast<int>(IFile::FileMode::Append)) {
            m_IsReadOnly = false;
            open_mode |= std::fstream::app;
        }
        if (mode & static_cast<int>(IFile::FileMode::Truncate)) {
            open_mode |= std::fstream::trunc;
        }
        
        m_Stream.open(GetFileInfo().AbsolutePath().c_str(), open_mode);
    }
    
    /*
     * Close file
     */
    virtual void Close() override
    {
        if (IsOpened()) {
            m_Stream.close();
        }
    }
    
    /*
     * Check is file ready for reading/writing
     */
    virtual bool IsOpened() const override
    {
        return m_Stream.is_open();
    }
    
    /*
     * Seek on a file
     */
    virtual uint64_t Seek(uint64_t offset, Origin origin) override
    {
        if (!IsOpened()) {
            return 0;
        }
        
        std::ios_base::seekdir dir = std::ios_base::beg;
        if (origin == IFile::Origin::End) {
            dir = std::ios_base::end;
        } else if (origin == IFile::Origin::Set) {
            dir = std::ios_base::cur;
        }
        
        m_Stream.seekg(offset, dir);
        if (m_Stream.fail()) {
            return 0;
        }

        return Tell();
    }
    /*
     * Returns offset in file
     */
    virtual uint64_t Tell() override
    {
        return static_cast<uint64_t>(m_Stream.tellg());
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
        if ((m_Mode & static_cast<int>(IFile::FileMode::Read)) == 0) {
            return 0;
        }
        
        m_Stream.read(reinterpret_cast<char*>(buffer), size);
        return static_cast<uint64_t>(m_Stream.gcount());
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
        if ((m_Mode & static_cast<int>(IFile::FileMode::Write)) == 0) {
            return 0;
        }
        
        m_Stream.write(reinterpret_cast<const char*>(buffer), size);
        return static_cast<uint64_t>(m_Stream.gcount());
    }
    
private:
    FileInfo m_FileInfo;
    std::fstream m_Stream;
    bool m_IsReadOnly;
    int m_Mode;
};
    
} // namespace vfspp

#endif /* NATIVEFILE_H */
