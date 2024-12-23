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
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
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
            std::lock_guard<std::mutex> lock(m_Mutex);
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
            std::lock_guard<std::mutex> lock(m_Mutex);
            return IsReadOnlyST();
        } else {
            return IsReadOnlyST();
        }
    }
    
    /*
     * Open file for reading/writting
     */
    virtual void Open(FileMode mode) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            OpenST(mode);
        } else {
            OpenST(mode);
        }
    }
    
    /*
     * Close file
     */
    virtual void Close() override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
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
            std::lock_guard<std::mutex> lock(m_Mutex);
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
            std::lock_guard<std::mutex> lock(m_Mutex);
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
            std::lock_guard<std::mutex> lock(m_Mutex);
            return TellST();
        } else {
            return TellST();
        }
    }
    
    /*
     * Read data from file to buffer
     */
    virtual uint64_t Read(uint8_t* buffer, uint64_t size) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return ReadST(buffer, size);
        } else {
            return ReadST(buffer, size);
        }
    }
    /*
     * Write buffer data to file
     */
    virtual uint64_t Write(const uint8_t* buffer, uint64_t size) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return WriteST(buffer, size);
        } else {
            return WriteST(buffer, size);
        }
    }

    /*
     * Read data from file to vector
     */
    virtual uint64_t Read(std::vector<uint8_t>& buffer, uint64_t size) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return ReadST(buffer, size);
        } else {
            return ReadST(buffer, size);
        }
    }
    
    /*
     * Write data from vector to file
     */
    virtual uint64_t Write(const std::vector<uint8_t>& buffer) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return WriteST(buffer);
        } else {
            return WriteST(buffer);
        }
    }
    
    /*
     * Read data from file to stream
     */
    virtual uint64_t Read(std::ostream& stream, uint64_t size, uint64_t bufferSize = 1024) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return ReadST(stream, size, bufferSize);
        } else {
            return ReadST(stream, size, bufferSize);
        }
    }
    
    /*
     * Write data from stream to file
     */
    virtual uint64_t Write(std::istream& stream, uint64_t size, uint64_t bufferSize = 1024) override
    {
        if constexpr (VFSPP_MT_SUPPORT_ENABLED) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return WriteST(stream, size, bufferSize);
        } else {
            return WriteST(stream, size, bufferSize);
        }
    }

private:
    inline const FileInfo& GetFileInfoST() const
    {
        return m_FileInfo;
    }
    
    inline uint64_t SizeST()
    {
        if (IsOpenedST()) {
            return m_Data.size();
        }
        
        return 0;
    }

    inline bool IsReadOnlyST() const
    {
        return m_IsReadOnly;
    }

    inline void OpenST(FileMode mode)
    {
        if (IsOpenedST() && m_Mode == mode) {
            SeekST(0, Origin::Begin);
            return;
        }
        
        m_Mode = mode;
        m_SeekPos = 0;
        m_IsReadOnly = true;
        
        if ((mode & FileMode::Write) == FileMode::Write) {
            m_IsReadOnly = false;
        }
        if ((mode & FileMode::Append) == FileMode::Append) {
            m_IsReadOnly = false;
            m_SeekPos = SizeST() > 0 ? SizeST() - 1 : 0;
        }
        if ((mode & FileMode::Truncate) == FileMode::Truncate) {
            m_Data.clear();
        }
        
        m_IsOpened = true;
    }

    inline void CloseST()
    {
        m_IsReadOnly = true;
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
        
        if (origin == Origin::Begin) {
            m_SeekPos = offset;
        } else if (origin == Origin::End) {
            m_SeekPos = Size() - offset;
        } else if (origin == Origin::Set) {
            m_SeekPos += offset;
        }
        m_SeekPos = std::min(m_SeekPos, Size() - 1);

        return TellST();
    }

    inline uint64_t TellST()
    {
        return m_SeekPos;
    }
    
    inline uint64_t ReadST(uint8_t* buffer, uint64_t size)
    {
        if (!IsOpenedST()) {
            return 0;
        }

        // Skip reading if file is not opened for reading
        if ((m_Mode & FileMode::Read) != FileMode::Read) {
            return 0;
        }
        
        uint64_t leftSize = SizeST() - TellST();
        uint64_t maxSize = std::min(size, leftSize);
        if (maxSize > 0) {
            memcpy(buffer, m_Data.data(), static_cast<size_t>(maxSize));
            return maxSize;
        }

        return 0;
    }

    inline uint64_t WriteST(const uint8_t* buffer, uint64_t size)
    {
        if (!IsOpenedST() || IsReadOnlyST()) {
            return 0;
        }
        
        // Skip writing if file is not opened for writing
        if ((m_Mode & FileMode::Write) != FileMode::Write) {
            return 0;
        }
        
        uint64_t leftSize = SizeST() - TellST();
        if (size > leftSize) {
            m_Data.resize((size_t)(m_Data.size() + (size - leftSize)));
        }
        memcpy(m_Data.data() + TellST(), buffer, static_cast<size_t>(size));
        
        return size;
    }

    inline uint64_t ReadST(std::vector<uint8_t>& buffer, uint64_t size)
    {
        buffer.resize(size);
        return ReadST(buffer.data(), size);
    }
    
    inline uint64_t WriteST(const std::vector<uint8_t>& buffer)
    {
        return WriteST(buffer.data(), buffer.size());
    }
    
    inline uint64_t ReadST(std::ostream& stream, uint64_t size, uint64_t bufferSize = 1024)
    {
        // read chunk of data from file and write it to stream untill all data is read
        uint64_t totalSize = size;
        std::vector<uint8_t> buffer(bufferSize);
        while (size > 0) {
            uint64_t bytesRead = ReadST(buffer.data(), std::min(size, static_cast<uint64_t>(buffer.size())));
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
    
    inline uint64_t WriteST(std::istream& stream, uint64_t size, uint64_t bufferSize = 1024)
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
			
			WriteST(buffer.data(), bytesRead);
			
			size -= bytesRead;
		}
		
		return totalSize - size;
    }
    
private:
    std::vector<uint8_t> m_Data;
    FileInfo m_FileInfo;
    bool m_IsReadOnly;
    bool m_IsOpened;
    uint64_t m_SeekPos;
    FileMode m_Mode;
    mutable std::mutex m_Mutex;
};
    
} // namespace vfspp

#endif // MEMORYFILE_HPP
