# vfspp

vfspp is a C++ Virtual File System header-only library that allows manipulation of files from memory, zip archives, or native filesystems as if they were part of a single native filesystem. This is particularly useful for game developers who want to use resources in the native filesystem during development and then pack them into an archive for distribution builds. The library is thread-safe, ensuring safe operations in multi-threaded environments. Note: This library requires C++20 or later.

## Usage Example

```C++
// Register native filesystem during development or zip for distribution build

auto vfs = std::make_shared<MultiThreadedVirtualFileSystem>(); // To create single threaded version, use SingleThreadedVirtualFileSystem

#if defined(DISTRIBUTION_BUILD)
	auto rootFS = vfs->CreateFileSystem<ZipFileSystem>("/", "Resources.zip");
#else
	auto rootFS = vfs->CreateFileSystem<NativeFileSystem>("/", GetBundlePath() + "Resources");
#endif

```

It's often useful to have several mounted filesystems. For example:
- "/" as your root native filesystem
- "/tmp" as a memory filesystem for working with temporary files without disk operations
- "/resources" as a zip filesystem with game resources

Here's an example of how to set up multiple filesystems:

```C++

auto vfs = std::make_shared<MultiThreadedVirtualFileSystem>();

if (!vfs->CreateFileSystem<NativeFileSystem>("/", GetBundlePath() + "Documents/")) {
	// Handle error
}
if (!vfs->CreateFileSystem<ZipFileSystem>("/resources", "Resources.zip")) {
	// Handle error
}
if (!vfs->CreateFileSystem<MemoryFileSystem>("/tmp")) {
	// Handle error
}

// Example: Open a save file
if (auto saveFile = vfs->OpenFile(FileInfo("/savefile.sav"), IFile::FileMode::Read)) {
	// Parse game save
	// ...
}

// Example: Work with a temporary file in memory
if (auto userAvatarFile = vfs->OpenFile(FileInfo("/tmp/avatar.jpg"), IFile::FileMode::ReadWrite)) {
	// Load avatar from network and store it in memory
	// ...
	userAvatarFile->Write(data.data(), data.size());
}

// Example: Load a resource from the zip file
if (auto textureFile = vfs->OpenFile(FileInfo("/resources/background.pvr"), IFile::FileMode::Read)) {
	// Create texture
	// ...
}
```

### Patching/DLC feature

vfspp supports patching/DLC feature. You can mount multiple filesystems to the same alias and access files merged from all filesystems. For example, you can mount the base game filesystem and the patch filesystem. If the file is present in the patch filesystem, it will be used; otherwise, the file from the base game filesystem will be used. Search order performed from the last mounted filesystem to the first filesystem. The first filesystem mounted to the alias will be the default filesystem, so creating a new file will be created in the first 'base' filesystem.

```C++
if (!vfs->CreateFileSystem<NativeFileSystem>("/dlc", "test-data/dlc1")) {
	std::fprintf(stderr, "Failed to mount native filesystem for dlc1\n");
	return 1;
}
	
if (IFilePtr dlcFile = vfs->OpenFile(FileInfo("/dlc/file.txt"), IFile::FileMode::Read)) {
	PrintFileContent("File /dlc/file.txt that exists in dlc1:", dlcFile);
}

if (!vfs->CreateFileSystem<NativeFileSystem>("/dlc", "test-data/dlc2")) {
	std::fprintf(stderr, "Failed to mount native filesystem for dlc2\n");
	return 1;
}

if (IFilePtr dlcFile = vfs->OpenFile(FileInfo("/dlc/file.txt"), IFile::FileMode::Read)) {
	PrintFileContent("File /dlc/file.txt patched by dlc2:", dlcFile);
}

if (IFilePtr dlcFile = vfs->OpenFile(FileInfo("/dlc/file1.txt"), IFile::FileMode::Read)) {
	PrintFileContent("File /dlc/file1.txt that exists only in dlc1:", dlcFile);
}

if (IFilePtr dlcFile = vfs->OpenFile(FileInfo("/dlc/file2.txt"), IFile::FileMode::Read)) {
	PrintFileContent("File /dlc/file2.txt that exists only in dlc2:", dlcFile);
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
- Add vfspp as dependency to your target and set C++20 standard
```cmake 
target_link_libraries(<your_project> PRIVATE vfspp)
target_compile_features(<your_project> PRIVATE cxx_std_20)
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