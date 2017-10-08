//
//  CZipFile.h
//  vfspp
//
//  Created by Yevgeniy Logachev on 6/23/16.
//
//

#ifndef CZIPFILE_H
#define CZIPFILE_H

#include "IFile.h"

namespace vfspp
{
CLASS_PTR(CZip);
    
class CZip
{
public:
    CZip(const std::string& zipPath);
    ~CZip();
    
    bool MapFile(const std::string& filename, std::vector<uint8_t>& data);
    const std::string& FileName() const;
    
    bool IsReadOnly() const;
    
private:
    std::string m_FileName;
    void* m_ZipArchive;
    typedef std::map<std::string, std::tuple<uint32_t, uint64_t>> TEntriesMap;
    static TEntriesMap s_Entries;
};
    

class CZipFile final : public IFile
{
    friend class CZipFileSystem;
public:
    CZipFile(const CFileInfo& fileInfo, CZipPtr zipFile);
    ~CZipFile();
    
    /*
     * Get file information
     */
    virtual const CFileInfo& FileInfo() const override;
    
    /*
     * Returns file size
     */
    virtual uint64_t Size() override;
    
    /*
     * Check is readonly filesystem
     */
    virtual bool IsReadOnly() const override;
    
    /*
     * Open existing file for reading, if not exists return null
     */
    virtual void Open(int mode) override;
    
    /*
     * Close file
     */
    virtual void Close() override;
    
    /*
     * Check is file ready for reading/writing
     */
    virtual bool IsOpened() const override;
    
    /*
     * Seek on a file
     */
    virtual uint64_t Seek(uint64_t offset, Origin origin) override;
    
    /*
     * Returns offset in file
     */
    virtual uint64_t Tell() override;
    
    /*
     * Read data from file to buffer
     */
    virtual uint64_t Read(uint8_t* buffer, uint64_t size) override;
    
    /*
     * Write buffer data to file
     */
    virtual uint64_t Write(const uint8_t* buffer, uint64_t size) override;
    
private:
    CZipPtr m_ZipArchive;
    std::vector<uint8_t> m_Data;
    CFileInfo m_FileInfo;
    bool m_IsReadOnly;
    bool m_IsOpened;
    bool m_HasChanges;
    uint64_t m_SeekPos;
    int m_Mode;
};
    
} // namespace vfspp

#endif /* CZIPFILE_H */
