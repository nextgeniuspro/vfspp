//
//  CMemoryFileSystem.cpp
//  vfspp
//
//  Created by Yevgeniy Logachev on 6/23/16.
//
//

#include "CMemoryFileSystem.h"
#include "CMemoryFile.h"
#include "CStringUtilsVFS.h"

using namespace vfspp;

// *****************************************************************************
// Constants
// *****************************************************************************

// *****************************************************************************
// Public Methods
// *****************************************************************************

CMemoryFileSystem::CMemoryFileSystem()
: m_IsInitialized(false)
{

}

CMemoryFileSystem::~CMemoryFileSystem()
{

}

void CMemoryFileSystem::Initialize()
{
    if (m_IsInitialized)
    {
        return;
    }
    
    m_IsInitialized = true;
}

void CMemoryFileSystem::Shutdown()
{
    m_FileList.clear();
    m_IsInitialized = false;
}


bool CMemoryFileSystem::IsInitialized() const
{
    return m_IsInitialized;
}


const std::string& CMemoryFileSystem::BasePath() const
{
    static std::string basePath = "/";
    return basePath;
}


const IFileSystem::TFileList& CMemoryFileSystem::FileList() const
{
    return m_FileList;
}


bool CMemoryFileSystem::IsReadOnly() const
{
    return false;
}


IFilePtr CMemoryFileSystem::OpenFile(const CFileInfo& filePath, int mode)
{
    CFileInfo fileInfo(BasePath(), filePath.AbsolutePath(), false);
    IFilePtr file = FindFile(fileInfo);
    bool isExists = (file != nullptr);
    if (!isExists)
    {
        file.reset(new CMemoryFile(fileInfo));
    }
    file->Open(mode);
    
    if (!isExists && file->IsOpened())
    {
        m_FileList.insert(file);
    }
    
    return file;
}


void CMemoryFileSystem::CloseFile(IFilePtr file)
{
    if (file)
    {
        file->Close();
    }
}


bool CMemoryFileSystem::CreateFile(const CFileInfo& filePath)
{
    bool result = false;
    if (!IsReadOnly() && !IsFileExists(filePath))
    {
        CFileInfo fileInfo(BasePath(), filePath.AbsolutePath(), false);
        IFilePtr file = OpenFile(filePath, IFile::Out | IFile::Truncate);
        if (file)
        {
            result = true;
            file->Close();
        }
    }
    else
    {
        result = true;
    }
    
    return result;
}


bool CMemoryFileSystem::RemoveFile(const CFileInfo& filePath)
{
    bool result = true;
    
    IFilePtr file = FindFile(filePath);
    if (!IsReadOnly() && file)
    {
        CFileInfo fileInfo(BasePath(), file->FileInfo().AbsolutePath(), false);
        m_FileList.erase(file);
    }
    
    return result;
}


bool CMemoryFileSystem::CopyFile(const CFileInfo& src, const CFileInfo& dest)
{
    bool result = false;
    if (!IsReadOnly())
    {
        CMemoryFilePtr srcFile = std::static_pointer_cast<CMemoryFile>(FindFile(src));
        CMemoryFilePtr dstFile = std::static_pointer_cast<CMemoryFile>(OpenFile(dest, IFile::Out));
        
        if (srcFile && dstFile)
        {
            dstFile->m_Data.assign(srcFile->m_Data.begin(), srcFile->m_Data.end());
            dstFile->Close();
            
            result = true;
        }
    }
    
    return result;
}


bool CMemoryFileSystem::RenameFile(const CFileInfo& src, const CFileInfo& dest)
{
    bool result = CopyFile(src, dest);
    
    if (result)
    {
        result = RemoveFile(src);
    }
    
    return result;
}

bool CMemoryFileSystem::IsFileExists(const CFileInfo& filePath) const
{
    return (FindFile(BasePath() + filePath.AbsolutePath()) != nullptr);
}


bool CMemoryFileSystem::IsFile(const CFileInfo& filePath) const
{
    IFilePtr file = FindFile(filePath);
    if (file)
    {
        return !file->FileInfo().IsDir();
    }
    
    return false;
}


bool CMemoryFileSystem::IsDir(const CFileInfo& dirPath) const
{
    IFilePtr file = FindFile(dirPath);
    if (file)
    {
        return file->FileInfo().IsDir();
    }
    
    return false;
}

// *****************************************************************************
// Protected Methods
// *****************************************************************************

// *****************************************************************************
// Private Methods
// *****************************************************************************

IFilePtr CMemoryFileSystem::FindFile(const CFileInfo& fileInfo) const
{
    TFileList::const_iterator it = std::find_if(m_FileList.begin(), m_FileList.end(), [&](IFilePtr file)
    {
        return file->FileInfo() == fileInfo;
    });
    
    if (it != m_FileList.end())
    {
        return *it;
    }
    
    return nullptr;
}
