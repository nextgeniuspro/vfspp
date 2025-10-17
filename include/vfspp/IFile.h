#ifndef VFSPP_IFILE_H
#define VFSPP_IFILE_H

#include "Global.h"
#include "FileInfo.hpp"

#include <span>

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
    [[nodiscard]]
    virtual const FileInfo& GetFileInfo() const = 0;
    
    /*
     * Returns file size
     */
    [[nodiscard]]
    virtual uint64_t Size() const = 0;

    /*
     * Check is readonly filesystem
     */
    [[nodiscard]]
    virtual bool IsReadOnly() const = 0;
    
    /*
     * Open file for reading/writing
     */
    [[nodiscard]]
    virtual bool Open(FileMode mode) = 0;
    
    /*
     * Close file
     */
    virtual void Close() = 0;
    
    /*
     * Check is file ready for reading/writing
     */
    [[nodiscard]]
    virtual bool IsOpened() const = 0;
    
    /*
     * Seek on a file
     */
    virtual uint64_t Seek(uint64_t offset, Origin origin) = 0;

    /*
     * Returns offset in file
     */
    [[nodiscard]]
    virtual uint64_t Tell() const = 0;

    /*
     * Read data from file to buffer
     */
    virtual uint64_t Read(std::span<uint8_t> buffer) = 0;

    /*
     * Read data from file to vector
     */
    virtual uint64_t Read(std::vector<uint8_t>& buffer, uint64_t size) = 0;

    /*
     * Write buffer data to file
     */
    virtual uint64_t Write(std::span<const uint8_t> buffer) = 0;
    
    /*
     * Write data from vector to file
     */
    virtual uint64_t Write(const std::vector<uint8_t>& buffer) = 0;

    /*
    * Helpers to check if mode has specific flag
    */
    static bool ModeHasFlag(FileMode mode, FileMode flag)
    {
        return (static_cast<uint8_t>(mode) & static_cast<uint8_t>(flag)) != 0;
    }

    /*
    * Validate if mode is correct
    */
    [[nodiscard]]
    static bool IsModeValid(FileMode mode)
    {
        // Must have at least read or write flag
        if (!ModeHasFlag(mode, FileMode::Read) && !ModeHasFlag(mode, FileMode::Write)) {
            return false;
        }

        // Append mode requires write flag
        if (ModeHasFlag(mode, FileMode::Append) && !ModeHasFlag(mode, FileMode::Write)) {
            return false;
        }

        // Truncate mode requires write flag
        if (ModeHasFlag(mode, FileMode::Truncate) && !ModeHasFlag(mode, FileMode::Write)) {
            return false;
        }

        return true;
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

#endif // VFSPP_IFILE_H
