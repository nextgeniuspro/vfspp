//
//  CVirtualFileSystem.cpp
//  vfspp
//
//  Created by Yevgeniy Logachev on 6/23/16.
//
//

#include "CVirtualFileSystem.h"
#include "CStringUtilsVFS.h"

using namespace vfspp;

// *****************************************************************************
// Constants
// *****************************************************************************

static CVirtualFileSystemPtr g_FS;

// *****************************************************************************
// Public Methods
// *****************************************************************************

struct SAliasComparator
{
    bool operator()(const CVirtualFileSystem::SSortedAlias& a1, const CVirtualFileSystem::SSortedAlias& a2) const
    {
        return a1.alias.length() > a2.alias.length();
    }
};

void vfspp::vfs_initialize()
{
    if (!g_FS)
    {
        g_FS.reset(new CVirtualFileSystem());
    }
}

void vfspp::vfs_shutdown()
{
    g_FS = nullptr;
}

CVirtualFileSystemPtr vfspp::vfs_get_global()
{
    return g_FS;
}

CVirtualFileSystem::CVirtualFileSystem()
{
}

CVirtualFileSystem::~CVirtualFileSystem()
{
    std::for_each(m_FileSystem.begin(), m_FileSystem.end(), [](const TFileSystemMap::value_type& fs)
    {
        fs.second->Shutdown();
    });
}

void CVirtualFileSystem::AddFileSystem(const std::string& alias, IFileSystemPtr filesystem)
{
    if (!filesystem)
    {
        return;
    }
    
    std::string a = alias;
    if (!CStringUtils::EndsWith(a, "/"))
    {
        a += "/";
    }
    
    TFileSystemMap::const_iterator it = m_FileSystem.find(a);
    if (it == m_FileSystem.end())
    {
        m_FileSystem[a] = filesystem;
        m_SortedAlias.push_back(SSortedAlias(a, filesystem));
        m_SortedAlias.sort(SAliasComparator());
    }
}

void CVirtualFileSystem::RemoveFileSystem(const std::string& alias)
{
    std::string a = alias;
    if (!CStringUtils::EndsWith(a, "/"))
    {
        a += "/";
    }
    
    TFileSystemMap::const_iterator it = m_FileSystem.find(a);
    if (it == m_FileSystem.end())
    {
        m_FileSystem.erase(it);
        // TODO: remove from alias list
    }
}

bool CVirtualFileSystem::IsFileSystemExists(const std::string& alias) const
{
    return (m_FileSystem.find(alias) != m_FileSystem.end());
}

IFilePtr CVirtualFileSystem::OpenFile(const CFileInfo& filePath, IFile::FileMode mode)
{
    IFilePtr file = nullptr;
    std::all_of(m_SortedAlias.begin(), m_SortedAlias.end(), [&](const TSortedAliasList::value_type& fs)
    {
        const std::string& alias = fs.alias;
        IFileSystemPtr filesystem = fs.filesystem;
        if (CStringUtils::StartsWith(filePath.BasePath(), alias) && filePath.AbsolutePath().length() != alias.length())
        {
            file = filesystem->OpenFile(filePath, mode);
        }
        
        if (file)
        {
            uintptr_t addr = reinterpret_cast<uintptr_t>(static_cast<void*>(file.get()));
            m_OpenedFiles[addr] = filesystem;
            
            return false;
        }
        
        return true;
    });
    
    return file;
}

void CVirtualFileSystem::CloseFile(IFilePtr file)
{
    uintptr_t addr = reinterpret_cast<uintptr_t>(static_cast<void*>(file.get()));
    
    std::unordered_map<uintptr_t, IFileSystemPtr>::const_iterator it = m_OpenedFiles.find(addr);
    if (it != m_OpenedFiles.end())
    {
        it->second->CloseFile(file);
        m_OpenedFiles.erase(it);
    }
}

// *****************************************************************************
// Protected Methods
// *****************************************************************************

// *****************************************************************************
// Private Methods
// *****************************************************************************
