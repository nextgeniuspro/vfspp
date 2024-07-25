#include "vfspp/VFS.h"

using namespace vfspp;

int main()
{
	VirtualFileSystemPtr vfs(new VirtualFileSystem());
    IFileSystemPtr rootFS(new NativeFileSystem("./files"));
    IFileSystemPtr memFS(new MemoryFileSystem());
    IFileSystemPtr zipFS(new ZipFileSystem("./test.zip"));

    rootFS->Initialize();
    memFS->Initialize();
    zipFS->Initialize();

    vfs->AddFileSystem("/", rootFS);
    vfs->AddFileSystem("/memory", memFS);
    vfs->AddFileSystem("/zip", zipFS);

    printf("Native filesystem test:\n");

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
        memset(data, 0, sizeof(data));
        file2->Read(reinterpret_cast<uint8_t*>(data), 256);
        
        printf("%s\n", data);
    }

    printf("Memory filesystem test:\n");

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
        memset(data, 0, sizeof(data));
        memFile->Read(reinterpret_cast<uint8_t*>(data), 256);
        
        printf("%s\n", data);
    }

    printf("Zip filesystem test:\n");

    IFileSystem::TFileList files = zipFS->FileList();
    for (auto& file : files)
	{
		printf("Zip file entry: %s\n", file->GetFileInfo().AbsolutePath().c_str());
	}

    IFilePtr zipFile = vfs->OpenFile(FileInfo("/zip/file.txt"), IFile::FileMode::Read);
    if (zipFile && zipFile->IsOpened())
    {
        char data[256];
        memset(data, 0, sizeof(data));
        zipFile->Read(reinterpret_cast<uint8_t*>(data), 256);

        printf("%s\n", data);
    }

	return 0;
}