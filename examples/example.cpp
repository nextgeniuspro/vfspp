#include "vfspp/VFS.h"


using namespace vfspp;
using namespace std::string_view_literals;

void PrintFile(const std::string& msg, IFilePtr file)
{
    if (file && file->IsOpened()) {
        std::string data(256, 0);
        file->Read(std::span<uint8_t>(reinterpret_cast<uint8_t*>(data.data()), data.size()));
        
        printf("%s\n%s\n", msg.c_str(), data.c_str());
    }
}

int main()
{
	VirtualFileSystemPtr vfs = std::make_shared<VirtualFileSystem>();
    // Paths now relative to working directory where executable resides with copied test-data
    IFileSystemPtr rootFS = std::make_shared<NativeFileSystem>("test-data/files");
    IFileSystemPtr memFS = std::make_shared<MemoryFileSystem>();
    IFileSystemPtr zipFS = std::make_shared<ZipFileSystem>("test-data/test.zip");

    rootFS->Initialize();
    memFS->Initialize();
    zipFS->Initialize();

    vfs->AddFileSystem("/", rootFS);
    vfs->AddFileSystem("/memory", memFS);
    vfs->AddFileSystem("/zip", zipFS);

    printf("Native filesystem test:\n");

    if (IFilePtr file = vfs->OpenFile(FileInfo("/test.txt"), IFile::FileMode::ReadWrite)) {
        std::string data = "The quick brown fox jumps over the lazy dog\n";
        file->Write(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }

    if (IFilePtr file2 = vfs->OpenFile(FileInfo("/test.txt"), IFile::FileMode::Read)) {
        PrintFile("File /test.txt:", file2);
    }

    printf("Memory filesystem test:\n");

    if (IFilePtr memFile = vfs->OpenFile(FileInfo("/memory/file.txt"), IFile::FileMode::ReadWrite)) {
        std::string data = "The quick brown fox jumps over the lazy dog\n";
        memFile->Write(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }
    
    if (IFilePtr memFile2 = vfs->OpenFile(FileInfo("/memory/file.txt"), IFile::FileMode::Read)) {
        PrintFile("File /memory/file.txt:", memFile2);
    }

    printf("Zip filesystem test:\n");

    IFileSystem::FilesList files = zipFS->FileList();
    for (auto& file : files) {
        printf("Zip file entry: %s\n", file.AbsolutePath().c_str());
    }

    if (IFilePtr zipFile = vfs->OpenFile(FileInfo("/zip/file.txt"), IFile::FileMode::Read)) {
        PrintFile("File /zip/file.txt:", zipFile);
    }

    printf("DLC filesystem test:\n");
    
    IFileSystemPtr dlc1FS(new NativeFileSystem("test-data/dlc1"));
    IFileSystemPtr dlc2FS(new NativeFileSystem("test-data/dlc2"));

    dlc1FS->Initialize();
    dlc2FS->Initialize();

    vfs->AddFileSystem("/dlc", dlc1FS);
       
    if (IFilePtr dlcFile = vfs->OpenFile(FileInfo("/dlc/file.txt"), IFile::FileMode::Read)) {
        PrintFile("File /dlc/file.txt that exists in dlc1:", dlcFile);
    }
    
    vfs->AddFileSystem("/dlc", dlc2FS);

    if (IFilePtr dlcFile = vfs->OpenFile(FileInfo("/dlc/file.txt"), IFile::FileMode::Read)) {
        PrintFile("File /dlc/file.txt patched by dlc2:", dlcFile);
    }

    if (IFilePtr dlcFile = vfs->OpenFile(FileInfo("/dlc/file1.txt"), IFile::FileMode::Read)) {
        PrintFile("File /dlc/file1.txt that exists only in dlc1:", dlcFile);
    }

    if (IFilePtr dlcFile = vfs->OpenFile(FileInfo("/dlc/file2.txt"), IFile::FileMode::Read)) {
        PrintFile("File /dlc/file2.txt that exists only in dlc2:", dlcFile);
    }

	return 0;
}

