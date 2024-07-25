#ifndef IFILE_H
#define IFILE_H

#include "VFS.h"
#include "FileInfo.hpp"

namespace vfspp
{
    
using IFilePtr = std::shared_ptr<class IFile>;
using IFileWeakPtr = std::weak_ptr<class IFile>;

class IFile
{
public:
    /*
     * Seek modes
     */
    enum class Origin
    {
        Begin,
        End,
        Set
    };
    
    /*
     * Open file mode
     */
    enum class FileMode
    {
        Read = (1 << 0),
        Write = (1 << 1),
        ReadWrite = Read | Write,
        Append = (1 << 2),
        Truncate = (1 << 3)
    };
    
public:
    IFile() = default;
    virtual ~IFile() = default;
    
    /*
     * Get file information
     */
    virtual const FileInfo& GetFileInfo() const = 0;
    
    /*
     * Returns file size
     */
    virtual uint64_t Size() = 0;
    
    /*
     * Check is readonly filesystem
     */
    virtual bool IsReadOnly() const = 0;
    
    /*
     * Open file for reading/writing
     */
    virtual void Open(int mode) = 0;
    
    /*
     * Close file
     */
    virtual void Close() = 0;
    
    /*
     * Check is file ready for reading/writing
     */
    virtual bool IsOpened() const = 0;
    
    /*
     * Seek on a file
     */
    virtual uint64_t Seek(uint64_t offset, Origin origin) = 0;

    /*
     * Returns offset in file
     */
    virtual uint64_t Tell() = 0;
    
    /*
     * Read data from file to buffer
     */
    virtual uint64_t Read(uint8_t* buffer, uint64_t size) = 0;

    /*
     * Write buffer data to file
     */
    virtual uint64_t Write(const uint8_t* buffer, uint64_t size) = 0;
    
    /*
     * Templated alternative to Read
     */
    template<typename T>
    bool Read(T& value)
    {
        return (Read(reinterpret_cast<uint8_t*>(&value), sizeof(value)) == sizeof(value));
    }
    
    /*
     * Templated alternative to Write
     */
    template<typename T>
    uint64_t Write(const T& value)
    {
        return (Write(reinterpret_cast<const uint8_t*>(&value), sizeof(value)) == sizeof(value));
    }

    /*
     * Read data from file to vector
     */
    virtual uint64_t Read(std::vector<uint8_t>& buffer, uint64_t size)
    {
        buffer.resize(size);
        return Read(buffer.data(), size);
    }
    
    /*
     * Write data from vector to file
     */
    virtual uint64_t Write(const std::vector<uint8_t>& buffer)
    {
        return Write(buffer.data(), buffer.size());
    }
    
    // /*
    //  * Read data from file to stream
    //  */
    // virtual uint64_t Read(std::istream& stream, uint64_t size)
    // {
    //     std::vector<uint8_t> buffer(size);
    //     uint64_t bytesRead = Read(buffer.data(), size);
    //     stream.write(reinterpret_cast<const char*>(buffer.data()), bytesRead);
    //     return bytesRead;
    // }
    
    // /*
    //  * Write data from stream to file
    //  */
    // virtual uint64_t Write(std::ostream& stream, uint64_t size)
    // {
    //     std::vector<uint8_t> buffer(size);
    //     stream.read(reinterpret_cast<char*>(buffer.data()), size);
    //     return Write(buffer.data(), stream.gcount());
    // }
};
    
inline bool operator ==(const IFile& f1, const IFile& f2)
{
    return f1.GetFileInfo() == f2.GetFileInfo();
}
    
inline bool operator ==(IFilePtr f1, IFilePtr f2)
{
    if (!f1 || !f2) {
        return false;
    }
    
    return f1->GetFileInfo() == f2->GetFileInfo();
}
    
} // namespace vfspp

#endif /* IFILE_H */
