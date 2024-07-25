#include "vfspp/VirtualFileSystem.hpp"
#include "vfspp/NativeFileSystem.hpp"
#include "vfspp/MemoryFileSystem.hpp"
// #include "CZipFileSystem.h"

using namespace vfspp;

int main()
{
	VirtualFileSystemPtr vfs(new VirtualFileSystem());
    IFileSystemPtr rootFS(new NativeFileSystem("./files"));
    IFileSystemPtr iconsFS(new NativeFileSystem("./icons"));
    IFileSystemPtr memFS(new MemoryFileSystem());

    rootFS->Initialize();
    iconsFS->Initialize();
    memFS->Initialize();

    vfs->AddFileSystem("/", rootFS);
    vfs->AddFileSystem("/icons", iconsFS);
    vfs->AddFileSystem("/memory", memFS);

    IFilePtr file = vfs->OpenFile(FileInfo("/test.txt"), IFile::FileMode::ReadWrite);
    if (file && file->IsOpened())
    {
        char data[] = "The quick brown fox jumps over the lazy dog\n";
        file->Write(reinterpret_cast<uint8_t*>(data), sizeof(data));
        file->Close();
    }

    IFilePtr file2 = vfs->OpenFile(FileInfo("/test.txt"), IFile::FileMode::Read);
    if (file2 && file2->IsOpened())
    {
        char data[256];
        file2->Read(reinterpret_cast<uint8_t*>(data), 256);
        
        printf("%s\n", data);
    }

    IFilePtr file3 = vfs->OpenFile(FileInfo("/icons/icon.png"), IFile::FileMode::Read);
    if (file3 && file3->IsOpened())
	{
		char data[256];
		file3->Read(reinterpret_cast<uint8_t*>(data), 256);
		
		printf("%s\n", data);
	}

    // Create memory file
    IFilePtr memFile = vfs->OpenFile(FileInfo("/memory/file.txt"), IFile::FileMode::ReadWrite);
    if (memFile && memFile->IsOpened())
    {
        char data[] = "The quick brown fox jumps over the lazy dog\n";
	    memFile->Write(reinterpret_cast<uint8_t*>(data), sizeof(data));
	    memFile->Close();
    }
    
    IFilePtr memFile2 = vfs->OpenFile(FileInfo("/memory/file.txt"), IFile::FileMode::Read);
    if (memFile2 && memFile2->IsOpened())
    {
        char data[256];
        memFile->Read(reinterpret_cast<uint8_t*>(data), 256);
        
        printf("%s\n", data);
    }

	// std::string zipPassword = "123";
    
    // IFileSystemPtr root_fs(new CNativeFileSystem("./"));
    // IFileSystemPtr zip_fs(new CZipFileSystem("password_123.zip", "/", true, zipPassword));
    // IFileSystemPtr mem_fs(new CMemoryFileSystem());
    
    // root_fs->Initialize();
    // zip_fs->Initialize();
    // mem_fs->Initialize();
    
    // CVirtualFileSystemPtr vfs = vfs_get_global();
    // vfs->AddFileSystem("/", root_fs);
    // vfs->AddFileSystem("/zip", zip_fs);
    // vfs->AddFileSystem("/memory/", mem_fs);

	// // Create memory file
    // IFilePtr memFile = vfs->OpenFile(CFileInfo("/memory/file.txt"), IFile::ReadWrite);
    // if (memFile && memFile->IsOpened())
    // {
    //     char data[] = "The quick brown fox jumps over the lazy dog\n";
	//     memFile->Write(reinterpret_cast<uint8_t*>(data), sizeof(data));
	//     memFile->Close();
    // }
    
    // IFilePtr memFile2 = vfs->OpenFile(CFileInfo("/memory/file.txt"), IFile::In);
    // if (memFile2 && memFile2->IsOpened())
    // {
    //     char data[256];
    //     memFile->Read(reinterpret_cast<uint8_t*>(data), 256);
        
    //     printf("%s\n", data);
    // }
    
    // // Create zip file
    // IFilePtr zipFile = vfs->OpenFile(CFileInfo("/zip/newFile.txt"), IFile::ReadWrite);
    // if (zipFile && zipFile->IsOpened())
    // {
    // 	char data[] = "The quick brown fox jumps over the lazy dog\n";
    // 	zipFile->Write(reinterpret_cast<uint8_t*>(data), sizeof(data));
    // 	zipFile->Close();
    // }

    // // Create native file
    // IFilePtr nativeFile = vfs->OpenFile(CFileInfo("/newFile.txt"), IFile::ReadWrite);
    // if (nativeFile && nativeFile->IsOpened())
    // {
    // 	char data[] = "The quick brown fox jumps over the lazy dog\n";
    // 	nativeFile->Write(reinterpret_cast<uint8_t*>(data), sizeof(data));
    // 	nativeFile->Close();
    // }

    // vfs_shutdown();

	return 0;
}