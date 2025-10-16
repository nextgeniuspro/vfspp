#include "vfspp/VFS.h"

#include <cstdio>
#include <span>


using namespace vfspp;
using namespace std::string_view_literals;

void PrintFileContent(const std::string& msg, IFilePtr file)
{
    if (file && file->IsOpened()) {
        std::string data(256, 0);
        file->Read(std::span<uint8_t>(reinterpret_cast<uint8_t*>(data.data()), data.size()));
        
        std::printf("%s\n%s\n", msg.c_str(), data.c_str());
    }
}

int main()
{
    auto vfs = std::make_shared<MultiThreadedVirtualFileSystem>(); // To create single threaded version, use SingleThreadedVirtualFileSystem
    
    // Native filesystem example
    printf("Native filesystem test:\n");

    if (!vfs->CreateFileSystem<NativeFileSystem>("/", "test-data/files")) {
        std::fprintf(stderr, "Failed to mount native filesystem\n");
        return 1;
    }

    if (IFilePtr file = vfs->OpenFile("/test.txt", IFile::FileMode::ReadWrite)) {
        std::string data = "The quick brown fox jumps over the lazy dog\n";
        file->Write(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }

    if (IFilePtr file2 = vfs->OpenFile("/test.txt", IFile::FileMode::Read)) {
        PrintFileContent("File /test.txt:", file2);
    }

    // Memory filesystem example
    printf("Memory filesystem test:\n");

    if (!vfs->CreateFileSystem<MemoryFileSystem>("/memory")) {
        std::fprintf(stderr, "Failed to mount memory filesystem\n");
        return 1;
    }

    if (IFilePtr memFile = vfs->OpenFile("/memory/file.txt", IFile::FileMode::ReadWrite)) {
        std::string data = "The quick brown fox jumps over the lazy dog\n";
        memFile->Write(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }
    
    if (IFilePtr memFile2 = vfs->OpenFile("/memory/file.txt", IFile::FileMode::Read)) {
        PrintFileContent("File /memory/file.txt:", memFile2);
    }

    // Zip filesystem example
    printf("Zip filesystem test:\n");

    auto zipFSResult = vfs->CreateFileSystem<ZipFileSystem>("/zip", "test-data/test.zip");
    if (!zipFSResult) {
        std::fprintf(stderr, "Failed to mount zip filesystem\n");
        return 1;
    }

    const auto& files = zipFSResult.value()->GetFilesList();
    for (const auto& file : files) {
        printf("Zip file entry: %s\n", file.VirtualPath().c_str());
    }

    if (IFilePtr zipFile = vfs->OpenFile("/zip/file.txt", IFile::FileMode::Read)) {
        PrintFileContent("File /zip/file.txt:", zipFile);
    }

    // DLC filesystem example
    printf("DLC filesystem test:\n");

    if (!vfs->CreateFileSystem<NativeFileSystem>("/dlc", "test-data/dlc1")) {
        std::fprintf(stderr, "Failed to mount native filesystem for dlc1\n");
        return 1;
    }
       
    if (IFilePtr dlcFile = vfs->OpenFile("/dlc/file.txt", IFile::FileMode::Read)) {
        PrintFileContent("File /dlc/file.txt that exists in dlc1:", dlcFile);
    }
    
    if (!vfs->CreateFileSystem<NativeFileSystem>("/dlc", "test-data/dlc2")) {
        std::fprintf(stderr, "Failed to mount native filesystem for dlc2\n");
        return 1;
    }

    if (IFilePtr dlcFile = vfs->OpenFile("/dlc/file.txt", IFile::FileMode::Read)) {
        PrintFileContent("File /dlc/file.txt patched by dlc2:", dlcFile);
    }

    if (IFilePtr dlcFile = vfs->OpenFile("/dlc/file1.txt", IFile::FileMode::Read)) {
        PrintFileContent("File /dlc/file1.txt that exists only in dlc1:", dlcFile);
    }

    if (IFilePtr dlcFile = vfs->OpenFile("/dlc/file2.txt", IFile::FileMode::Read)) {
        PrintFileContent("File /dlc/file2.txt that exists only in dlc2:", dlcFile);
    }

	return 0;
}

