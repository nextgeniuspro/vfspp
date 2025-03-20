#include "vfspp/VFS.h"

using namespace vfspp;
using namespace std::string_view_literals;

void PrintFile(const std::string& msg, IFilePtr file)
{
    if (file && file->IsOpened()) {
        char data[256];
        memset(data, 0, sizeof(data));
        file->Read(reinterpret_cast<uint8_t*>(data), 256);
        
        printf("%s\n%s\n", msg.c_str(), data);
    }
}

int main()
{
	VirtualFileSystemPtr vfs(new VirtualFileSystem());
    IFileSystemPtr rootFS(new NativeFileSystem("../test-data/files"));
    IFileSystemPtr memFS(new MemoryFileSystem());
    IFileSystemPtr zipFS(new ZipFileSystem("../test-data/test.zip"));

    rootFS->Initialize();
    memFS->Initialize();
    zipFS->Initialize();

    vfs->AddFileSystem("/", rootFS);
    vfs->AddFileSystem("/memory", memFS);
    vfs->AddFileSystem("/zip", zipFS);

    printf("Native filesystem test:\n");

	auto test = vfs->AbsolutePath("/test.txt"sv);
    IFilePtr file = vfs->OpenFile(FileInfo("/test.txt"), IFile::FileMode::ReadWrite);
    if (file && file->IsOpened()) {
        char data[] = "The quick brown fox jumps over the lazy dog\n";
        file->Write(reinterpret_cast<uint8_t*>(data), sizeof(data));
        file->Close();
    }

    IFilePtr file2 = vfs->OpenFile(FileInfo("/test.txt"), IFile::FileMode::Read);
    if (file2 && file2->IsOpened()) {
        PrintFile("File /test.txt:", file2);
    }

    printf("Memory filesystem test:\n");

    IFilePtr memFile = vfs->OpenFile(FileInfo("/memory/file.txt"), IFile::FileMode::ReadWrite);
    if (memFile && memFile->IsOpened()) {
        char data[] = "The quick brown fox jumps over the lazy dog\n";
	    memFile->Write(reinterpret_cast<uint8_t*>(data), sizeof(data));
	    memFile->Close();
    }
    
    IFilePtr memFile2 = vfs->OpenFile(FileInfo("/memory/file.txt"), IFile::FileMode::Read);
    if (memFile2 && memFile2->IsOpened()) {
        PrintFile("File /memory/file.txt:", memFile2);
    }

    printf("Zip filesystem test:\n");

    IFileSystem::TFileList files = zipFS->FileList();
    for (auto& file : files) {
		printf("Zip file entry: %s\n", file.first.c_str());
	}

    IFilePtr zipFile = vfs->OpenFile(FileInfo("/zip/file.txt"), IFile::FileMode::Read);
    if (zipFile && zipFile->IsOpened()) {
        PrintFile("File /zip/file.txt:", zipFile);
    }

    printf("DLC filesystem test:\n");
    
    IFileSystemPtr dlc1FS(new NativeFileSystem("../test-data/dlc1"));
    IFileSystemPtr dlc2FS(new NativeFileSystem("../test-data/dlc2"));

    dlc1FS->Initialize();
    dlc2FS->Initialize();

    vfs->AddFileSystem("/dlc", dlc1FS);
       
    IFilePtr dlcFile = vfs->OpenFile(FileInfo("/dlc/file.txt"), IFile::FileMode::Read);
    if (dlcFile && dlcFile->IsOpened()) {
        PrintFile("File /dlc/file.txt that exists in dlc1:", dlcFile);
        dlcFile->Close();
    }
    
    vfs->AddFileSystem("/dlc", dlc2FS);

    dlcFile = vfs->OpenFile(FileInfo("/dlc/file.txt"), IFile::FileMode::Read);
    if (dlcFile && dlcFile->IsOpened()) {
        PrintFile("File /dlc/file.txt patched by dlc2:", dlcFile);
        dlcFile->Close();
    }

    IFilePtr dlcFile1 = vfs->OpenFile(FileInfo("/dlc/file1.txt"), IFile::FileMode::Read);
    if (dlcFile1 && dlcFile1->IsOpened()) {
        PrintFile("File /dlc/file1.txt that exists only in dlc1:", dlcFile1);
        dlcFile1->Close();
    }

    IFilePtr dlcFile2 = vfs->OpenFile(FileInfo("/dlc/file2.txt"), IFile::FileMode::Read);
    if (dlcFile2 && dlcFile2->IsOpened()) {
        PrintFile("File /dlc/file2.txt that exists only in dlc2:", dlcFile2);
        dlcFile2->Close();
    }

	return 0;
}

