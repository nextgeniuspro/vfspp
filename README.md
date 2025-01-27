# vfspp

vfspp is a C++ Virtual File System header-only library that allows manipulation of files from memory, zip archives, or native filesystems as if they were part of a single native filesystem. This is particularly useful for game developers who want to use resources in the native filesystem during development and then pack them into an archive for distribution builds. The library is thread-safe, ensuring safe operations in multi-threaded environments. Note: This library requires C++17 or later.

## Usage Example

```C++
// Register native filesystem during development or zip for distribution build

IFileSystemPtr rootFS = nullptr;
#if defined(DISTRIBUTION_BUILD)
	rootFS = std::make_unique<ZipFileSystem>("Resources.zip");
#else
	rootFS = std::make_unique<NativeFileSystem>(GetBundlePath() + "Resources");
#endif

rootFS->Initialize();

VirtualFileSystemPtr vfs(new VirtualFileSystem());
vfs->AddFileSystem("/", std::move(rootFS));
```

It's often useful to have several mounted filesystems. For example:
- "/" as your root native filesystem
- "/tmp" as a memory filesystem for working with temporary files without disk operations
- "/resources" as a zip filesystem with game resources

Here's an example of how to set up multiple filesystems:

```C++
// add 'VFSPP_ENABLE_MULTITHREADING' preprocessor definition to enable thread-safe operations
#define VFSPP_ENABLE_MULTITHREADING 

auto rootFS = std::make_unique<NativeFileSystem>(GetBundlePath() + "Documents/");
auto zipFS = std::make_unique<ZipFileSystem>("Resources.zip");
auto memFS = std::make_unique<MemoryFileSystem>();

rootFS->Initialize();
zipFS->Initialize();
memFS->Initialize();

VirtualFileSystemPtr vfs(new VirtualFileSystem());
vfs->AddFileSystem("/", std::move(rootFS));
vfs->AddFileSystem("/resources", std::move(zipFS));
vfs->AddFileSystem("/tmp", std::move(memFS));

// Example: Open a save file
if (auto saveFile = vfs->OpenFile(FileInfo("/savefile.sav"), IFile::FileMode::Read))
{
	if (saveFile->IsOpened()) {
		// Parse game save
		// ...
	}
}

// Example: Work with a temporary file in memory
if (auto userAvatarFile = vfs->OpenFile(FileInfo("/tmp/avatar.jpg"), IFile::FileMode::ReadWrite))
{
	if (userAvatarFile->IsOpened()) {
		// Load avatar from network and store it in memory
		// ...
		userAvatarFile->Write(data.data(), data.size());
		userAvatarFile->Close();
	}
}

// Example: Load a resource from the zip file
if (auto textureFile = vfs->OpenFile(FileInfo("/resources/background.pvr"), IFile::FileMode::Read))
{
	if (textureFile->IsOpened()) {
		// Create texture
		// ...
	}
}
```

### Patching/DLC feature

vfspp supports patching/DLC feature. You can mount multiple filesystems to the same alias and access files merged from all filesystems. For example, you can mount the base game filesystem and the patch filesystem. If the file is present in the patch filesystem, it will be used; otherwise, the file from the base game filesystem will be used. Search order performed from the last mounted filesystem to the first filesystem. The first filesystem mounted to the alias will be the default filesystem, so creating a new file will be created in the first 'base' filesystem.

```C++
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
```

## How To Integrate with cmake

- Add vfspp as submodule to your project
```bash
git submodule add https://github.com/nextgeniuspro/vfspp.git vendor/vfspp
```
- Update submodules to download dependencies
```bash
git submodule update --init --recursive
```
- Add vfspp as subdirectory to your CMakeLists.txt
```cmake 
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vendor/vfspp vfspp_build)
```
- Add vfspp as dependency to your target and set C++17 standard
```cmake 
target_link_libraries(<your_project> PRIVATE vfspp)
target_compile_features(<your_project> PRIVATE cxx_std_17)
```
- To add multi-threading support, add 'VFSPP_ENABLE_MULTITHREADING' preprocessor definition to your target
```cmake
add_compile_definitions(VFSPP_ENABLE_MULTITHREADING)
```

See examples/CMakeLists.txt for example of usage

## How To Build Example #

- Run cmake to generate project windows project files
```bash
cmake -B ./build -G "Visual Studio 17 2022" . -DBUILD_EXAMPLES=1
``` 
or to generate Xcode project files
```bash
cmake -B ./build -G "Xcode" . -DBUILD_EXAMPLES=1
```

- Open generated project files and build the target `vfsppexample`