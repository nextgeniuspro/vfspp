//
//  CNativeFile.h
//  vfspp
//
//  Created by Yevgeniy Logachev on 6/23/16.
//
//

#ifndef CNATIVEFILE_H
#define CNATIVEFILE_H

#include "IFile.h"

namespace vfspp
{

class CNativeFile final : public IFile
{
public:
    CNativeFile(const CFileInfo& fileInfo);
    ~CNativeFile();
    
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
     * Open file for reading/writting
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
    CFileInfo m_FileInfo;
    std::fstream m_Stream;
    bool m_IsReadOnly;
    int m_Mode;
};
    
} // namespace vfspp

#endif /* CNATIVEFILE_H */
