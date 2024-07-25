#ifndef IFILE_H
#define IFILE_H

#include "Global.h"
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
    enum class FileMode : uint8_t
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
    virtual void Open(FileMode mode) = 0;
    
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
    
    /*
     * Read data from file to stream
     */
    virtual uint64_t Read(std::ostream& stream, uint64_t size, uint64_t bufferSize = 1024)
    {
        // read chunk of data from file and write it to stream untill all data is read
        uint64_t totalSize = size;
        std::vector<uint8_t> buffer(bufferSize);
        while (size > 0) {
            uint64_t bytesRead = Read(buffer.data(), std::min(size, static_cast<uint64_t>(buffer.size())));
			if (bytesRead == 0) {
				break;
			}

            if (size < bytesRead) {
				bytesRead = size;
			}
			
			stream.write(reinterpret_cast<char*>(buffer.data()), bytesRead);

            size -= bytesRead;          
		}
        
        return totalSize - size;
    }
    
    /*
     * Write data from stream to file
     */
    virtual uint64_t Write(std::istream& stream, uint64_t size, uint64_t bufferSize = 1024)
    {
        // write chunk of data from stream to file untill all data is written
        uint64_t totalSize = size;
        std::vector<uint8_t> buffer(bufferSize);
        while (size > 0) {
			stream.read(reinterpret_cast<char*>(buffer.data()), std::min(size, static_cast<uint64_t>(buffer.size())));
			uint64_t bytesRead = stream.gcount();
			if (bytesRead == 0) {
				break;
			}
			
			if (size < bytesRead) {
				bytesRead = size;
			}
			
			Write(buffer.data(), bytesRead);
			
			size -= bytesRead;
		}
		
		return totalSize - size;
    }
};
    
inline bool operator==(IFilePtr f1, IFilePtr f2)
{
    if (!f1 || !f2) {
        return false;
    }
    
    return f1->GetFileInfo() == f2->GetFileInfo();
}

// Overload bitwise operators for FileMode enum class
constexpr IFile::FileMode operator|(IFile::FileMode lhs, IFile::FileMode rhs) {
    return static_cast<IFile::FileMode>(
        static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

constexpr IFile::FileMode operator&(IFile::FileMode lhs, IFile::FileMode rhs) {
    return static_cast<IFile::FileMode>(
        static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

constexpr IFile::FileMode operator^(IFile::FileMode lhs, IFile::FileMode rhs) {
    return static_cast<IFile::FileMode>(
        static_cast<uint8_t>(lhs) ^ static_cast<uint8_t>(rhs));
}

constexpr IFile::FileMode operator~(IFile::FileMode perm) {
    return static_cast<IFile::FileMode>(~static_cast<uint8_t>(perm));
}
    
} // namespace vfspp

#endif /* IFILE_H */
