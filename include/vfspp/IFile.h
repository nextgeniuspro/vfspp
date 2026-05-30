#ifndef VFSPP_IFILE_H
#define VFSPP_IFILE_H

#include "Global.h"
#include "EntryInfo.hpp"


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
    virtual const EntryInfo& GetEntryInfo() const = 0;
    
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
     * Typed Read overload: reads sizeof(T) * buffer.size() bytes into a span of any
     * trivially-copyable T. 
     */
    template <typename T> 
    requires std::is_trivially_copyable_v<T> && (!std::is_same_v<std::remove_cv_t<T>, uint8_t>)
    uint64_t Read(std::span<T> buffer)
    {
        auto* p = reinterpret_cast<uint8_t*>(buffer.data());
        return Read(std::span<uint8_t>(p, buffer.size_bytes()));
    }

    /*
     * Typed Read overload: resizes the destination vector to `count` elements
     * and reads count * sizeof(T) bytes.
     */
    template <typename T>
    requires std::is_trivially_copyable_v<T> && (!std::is_same_v<std::remove_cv_t<T>, uint8_t>)
    uint64_t Read(std::vector<T>& buffer, uint64_t count)
    {
        buffer.resize(static_cast<std::size_t>(count));
        auto* p = reinterpret_cast<uint8_t*>(buffer.data());
        return Read(std::span<uint8_t>(p, buffer.size() * sizeof(T)));
    }

    /*
     * Typed Write overload: writes sizeof(T) * buffer.size() bytes from a span of any
     * trivially-copyable T.
     */
    template <typename T>
    requires std::is_trivially_copyable_v<T> && (!std::is_same_v<std::remove_cv_t<T>, uint8_t>)
    uint64_t Write(std::span<const T> buffer)
    {
        const auto* p = reinterpret_cast<const uint8_t*>(buffer.data());
        return Write(std::span<const uint8_t>(p, buffer.size_bytes()));
    }

    /*
     * Typed Write overload: writes a vector of any trivially-copyable T.
     */
    template <typename T>
    requires std::is_trivially_copyable_v<T> && (!std::is_same_v<std::remove_cv_t<T>, uint8_t>)
    uint64_t Write(const std::vector<T>& buffer)
    {
        const auto* p = reinterpret_cast<const uint8_t*>(buffer.data());
        return Write(std::span<const uint8_t>(p, buffer.size() * sizeof(T)));
    }

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
    
    return f1->GetEntryInfo() == f2->GetEntryInfo();
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
