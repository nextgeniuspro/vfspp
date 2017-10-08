//
//  CMemoryFile.cpp
//  vfspp
//
//  Created by Yevgeniy Logachev on 6/23/16.
//
//

#include "CMemoryFile.h"
#include <cstring>

using namespace vfspp;

// *****************************************************************************
// Constants
// *****************************************************************************

// *****************************************************************************
// Public Methods
// *****************************************************************************

CMemoryFile::CMemoryFile(const CFileInfo& fileInfo)
: m_FileInfo(fileInfo)
, m_IsReadOnly(true)
, m_IsOpened(false)
, m_SeekPos(0)
, m_Mode(0)
{
    
}

CMemoryFile::~CMemoryFile()
{
    Close();
}

const CFileInfo& CMemoryFile::FileInfo() const
{
    return m_FileInfo;
}

uint64_t CMemoryFile::Size()
{
    if (IsOpened())
    {
        return m_Data.size();
    }
    
    return 0;
}

bool CMemoryFile::IsReadOnly() const
{
    return m_IsReadOnly;
}

void CMemoryFile::Open(int mode)
{
    if (IsOpened() && m_Mode == mode)
    {
        Seek(0, IFile::Begin);
        return;
    }
    
    m_Mode = mode;
    m_SeekPos = 0;
    m_IsReadOnly = true;
    
    if (mode & (int)IFile::Out)
    {
        m_IsReadOnly = false;
    }
    if (mode & (int)IFile::Append)
    {
        m_IsReadOnly = false;
        m_SeekPos = Size() > 0 ? Size() - 1 : 0;
    }
    if (mode & (int)IFile::Truncate)
    {
        m_Data.clear();
    }
    
    m_IsOpened = true;
}

void CMemoryFile::Close()
{
    m_IsReadOnly = true;
    m_IsOpened = false;
    m_SeekPos = 0;
}

bool CMemoryFile::IsOpened() const
{
    return m_IsOpened;
}

uint64_t CMemoryFile::Seek(uint64_t offset, Origin origin)
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

uint64_t CMemoryFile::Tell()
{
    return m_SeekPos;
}

uint64_t CMemoryFile::Read(uint8_t* buffer, uint64_t size)
{
    if (!IsOpened())
    {
        return 0;
    }
    
    uint64_t bufferSize = Size() - Tell();
    uint64_t maxSize = std::min(size, bufferSize);
    if (maxSize > 0)
    {
        memcpy(buffer, m_Data.data(), (size_t)maxSize);
    }
    else
    {
        return 0;
    }
    
    return maxSize;
}

uint64_t CMemoryFile::Write(const uint8_t* buffer, uint64_t size)
{
    if (!IsOpened() || IsReadOnly())
    {
        return 0;
    }
    
    uint64_t bufferSize = Size() - Tell();
    if (size > bufferSize)
    {
        m_Data.resize((size_t)(m_Data.size() + (size - bufferSize)));
    }
    memcpy(m_Data.data() + Tell(), buffer, (size_t)size);
    
    return size;
}

// *****************************************************************************
// Protected Methods
// *****************************************************************************

// *****************************************************************************
// Private Methods
// *****************************************************************************
