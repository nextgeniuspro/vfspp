//
//  CZipFile.cpp
//  vfspp
//
//  Created by Yevgeniy Logachev on 6/23/16.
//
//

#include "CZipFile.h"
#include "zip.h"
#include "unzip.h"
#include <unistd.h>
#include "CStringUtils.h"

using namespace vfspp;

// *****************************************************************************
// Constants
// *****************************************************************************

// *****************************************************************************
// Public Methods
// *****************************************************************************

CZip::CZip(const std::string& zipPath, bool needCreate, const std::string& password)
: m_FileName(zipPath)
, m_Password(password)
, m_ZipArchive(nullptr)
, m_UnzFile(nullptr)
{
    if (needCreate && access(m_FileName.c_str(), F_OK) == -1)
    {
        void* zipCreated = zipOpen64(FileName().c_str(), APPEND_STATUS_CREATE);
        if (zipCreated)
        {
            zip_fileinfo zi = {{0}};
            time_t rawtime;
            time (&rawtime);
            auto timeinfo = localtime(&rawtime);
            zi.tmz_date.tm_sec = timeinfo->tm_sec;
            zi.tmz_date.tm_min = timeinfo->tm_min;
            zi.tmz_date.tm_hour = timeinfo->tm_hour;
            zi.tmz_date.tm_mday = timeinfo->tm_mday;
            zi.tmz_date.tm_mon = timeinfo->tm_mon;
            zi.tmz_date.tm_year = timeinfo->tm_year;
            
            int err = -1;
            if (Password().empty())
            {
                err = zipOpenNewFileInZip(zipCreated, NULL, &zi,
                                          NULL, 0,
                                          NULL, 0,
                                          NULL,
                                          Z_DEFLATED,
                                          Z_DEFAULT_COMPRESSION);
            }
            else
            {
                err = zipOpenNewFileInZip3(zipCreated, NULL, &zi,
                                           NULL, 0,
                                           NULL, 0,
                                           NULL, //comment
                                           Z_DEFLATED,
                                           Z_DEFAULT_COMPRESSION,
                                           0 ,
                                           15 ,
                                           8 ,
                                           Z_DEFAULT_STRATEGY,
                                           Password().c_str(),
                                           0);
            }
            
            zipClose(zipCreated, 0);
        }
    }
}

CZip::~CZip()
{
    Unlock();
}

const std::string& CZip::FileName() const
{
    return m_FileName;
}

const std::string& CZip::Password() const
{
    return m_Password;
}

void* CZip::ZipOpen()
{
    UnzClose();
    
    if (!m_ZipArchive)
    {
        m_ZipArchive = zipOpen64(FileName().c_str(), APPEND_STATUS_ADDINZIP);
    }
    return m_ZipArchive;
};

void* CZip::UnzOpen()
{
    ZipClose();
    
    if (!m_UnzFile)
    {
        m_UnzFile = unzOpen64(FileName().c_str());
    }
    return m_UnzFile;
};

void CZip::ZipClose()
{
    if (m_ZipArchive)
    {
        zipClose(m_ZipArchive, 0);
        m_ZipArchive = nullptr;
    }
}

void CZip::UnzClose()
{
    if (m_UnzFile)
    {
        unzClose(m_UnzFile);
        m_UnzFile = nullptr;
    }
}

void CZip::Lock()
{
    m_Mutex.lock();
};
void CZip::Unlock()
{
    m_Mutex.unlock();
}

bool CZip::IsReadOnly() const
{
    return access(FileName().c_str(), W_OK);
}


CZipFile::CZipFile(const CFileInfo& fileInfo, CZipPtr zipFile)
: m_ZipArchive(zipFile)
, m_FileInfo(fileInfo)
, m_IsReadOnly(true)
, m_IsOpened(false)
, m_HasChanges(false)
, m_SeekPos(0)
, m_Mode(0)
{
}

CZipFile::~CZipFile()
{
    
}

const CFileInfo& CZipFile::FileInfo() const
{
    return m_FileInfo;
}

uint64_t CZipFile::Size()
{
    if (IsOpened())
    {
        return m_Data.size();
    }
    
    return 0;
}

bool CZipFile::IsReadOnly() const
{
    return (m_ZipArchive->IsReadOnly() && m_IsReadOnly);
}

void CZipFile::Open(int mode)
{
    if (!FileInfo().IsValid() || (IsOpened() && m_Mode == mode))
    {
        return;
    }
    
    m_ZipArchive->Lock();
    unzFile unz = m_ZipArchive->UnzOpen();
    assert(unz);
    
    std::string absPath = FileInfo().AbsolutePath();
    if (CStringUtils::StartsWith(absPath, "/"))
    {
        absPath = absPath.substr(1, absPath.length() - 1);
    }
    
    bool entryOpen = false;
    int err = unzLocateFile(unz, absPath.c_str(), 0);
    if (err == UNZ_OK)
    {
        if (m_ZipArchive->Password().empty())
        {
            err = unzOpenCurrentFile(unz);
        }
        else
        {
            err = unzOpenCurrentFilePassword(unz, m_ZipArchive->Password().c_str());
        }
        entryOpen = (err == UNZ_OK);
        
        unz_file_info64 oFileInfo;
        int err = unzGetCurrentFileInfo64(unz, &oFileInfo, 0, 0, 0, 0, 0, 0);
        if (err == UNZ_OK)
        {
            uint64_t size = oFileInfo.uncompressed_size;
            m_Data.resize(size);
            size = unzReadCurrentFile(unz, m_Data.data(), (unsigned int)size);
        }
    }
    
    if (entryOpen)
    {
        unzCloseCurrentFile(unz);
    }
    m_ZipArchive->Unlock();
    
    m_Mode = mode;
    m_IsReadOnly = true;
    m_SeekPos = 0;
    if (mode & IFile::Out)
    {
        m_IsReadOnly = false;
    }
    if (mode & IFile::Append)
    {
        m_IsReadOnly = false;
        m_SeekPos = Size() > 0 ? Size() - 1 : 0;
    }
    if (mode & IFile::Truncate)
    {
        m_Data.clear();
    }
    
    m_IsOpened = true;
}

void CZipFile::Close()
{
    if (IsReadOnly() || !m_HasChanges)
    {
        m_IsOpened = false;
        return;
    }
    
    m_ZipArchive->Lock();
    unzFile zip = m_ZipArchive->ZipOpen();
    assert(zip);
    
    std::string absPath = FileInfo().AbsolutePath();
    bool entryOpen = false;
    if (CStringUtils::StartsWith(absPath, "/"))
    {
        absPath = absPath.substr(1, absPath.length() - 1);
    }
    
    zip_fileinfo zi = {{0}};
    time_t rawtime;
    time (&rawtime);
    auto timeinfo = localtime(&rawtime);
    zi.tmz_date.tm_sec = timeinfo->tm_sec;
    zi.tmz_date.tm_min = timeinfo->tm_min;
    zi.tmz_date.tm_hour = timeinfo->tm_hour;
    zi.tmz_date.tm_mday = timeinfo->tm_mday;
    zi.tmz_date.tm_mon = timeinfo->tm_mon;
    zi.tmz_date.tm_year = timeinfo->tm_year;
    
    int err = -1;
    if (m_ZipArchive->Password().empty())
    {
        err = zipOpenNewFileInZip(zip, absPath.c_str(), &zi,
                                  NULL, 0,
                                  NULL, 0,
                                  NULL,
                                  Z_DEFLATED,
                                  Z_DEFAULT_COMPRESSION);
    }
    else
    {
        uLong crcValue = crc32(0L, NULL, 0L);
        crcValue = crc32(crcValue, (const Bytef*)m_Data.data(), (unsigned int)m_Data.size());
        err = zipOpenNewFileInZip3(zip, absPath.c_str(), &zi,
                                   NULL, 0,
                                   NULL, 0,
                                   NULL, //comment
                                   Z_DEFLATED,
                                   Z_DEFAULT_COMPRESSION,
                                   0 ,
                                   15 ,
                                   8 ,
                                   Z_DEFAULT_STRATEGY,
                                   m_ZipArchive->Password().c_str(),
                                   crcValue);
    }
    entryOpen = (err == ZIP_OK);
    if (entryOpen)
    {
        if (Size() > 0)
        {
            err = zipWriteInFileInZip(zip, m_Data.data(), (unsigned int)m_Data.size());
        }
        
        zipCloseFileInZip(zip);
    }
    m_ZipArchive->ZipClose();
    m_ZipArchive->Unlock();
    
    m_IsOpened = false;
}

bool CZipFile::IsOpened() const
{
    return m_IsOpened;
}

uint64_t CZipFile::Seek(uint64_t offset, Origin origin)
{
    if (!IsOpened())
    {
        return 0;
    }
    
    if (origin == IFile::Begin)
    {
        m_SeekPos = offset;
    }
    else if (origin == IFile::End)
    {
        m_SeekPos = Size() - offset;
    }
    else
    {
        m_SeekPos += offset;
    }
    m_SeekPos = std::min(m_SeekPos, Size() - 1);
    
    return Tell();
}

uint64_t CZipFile::Tell()
{
    return m_SeekPos;
}

uint64_t CZipFile::Read(uint8_t* buffer, uint64_t size)
{
    if (!IsOpened())
    {
        return 0;
    }
    
    uint64_t bufferSize = Size() - Tell();
    uint64_t maxSize = std::min(size, bufferSize);
    if (maxSize > 0)
    {
        memcpy(buffer, m_Data.data(), maxSize);
    }
    else
    {
        return 0;
    }
    
    return maxSize;
}

uint64_t CZipFile::Write(const uint8_t* buffer, uint64_t size)
{
    if (!IsOpened() || IsReadOnly())
    {
        return 0;
    }
    
    uint64_t bufferSize = Size() - Tell();
    if (size > bufferSize)
    {
        m_Data.resize(m_Data.size() + (size - bufferSize));
    }
    memcpy(m_Data.data() + Tell(), buffer, size);
    
    m_HasChanges = true;
    
    return size;
}

// *****************************************************************************
// Protected Methods
// *****************************************************************************

// *****************************************************************************
// Private Methods
// *****************************************************************************