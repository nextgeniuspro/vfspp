# vfspp

vfspp is a C++ Virtual File System header-only library that allows manipulation of files from memory, zip archives, or native filesystems as if they were part of a single native filesystem. This is particularly useful for game developers who want to use resources in the native filesystem during development and then pack them into an archive for distribution builds. Note: This library requires C++17 or later.

## Usage Example

```C++
// Register native filesystem during development or zip for distribution build

IFileSystemPtr root_fs = nullptr;
#if defined(DISTRIBUTION_BUILD)
	root_fs = std::make_unique<CZipFileSystem>("Resources.zip", "/");
#else
	root_fs = std::make_unique<CNativeFileSystem>(GetBundlePath() + "Resources/");
#endif

root_fs->Initialize();

VirtualFileSystemPtr vfs(new VirtualFileSystem());
vfs->AddFileSystem("/", std::move(root_fs));
```

It's often useful to have several mounted filesystems. For example:
- "/" as your root native filesystem
- "/tmp" as a memory filesystem for working with temporary files without disk operations
- "/resources" as a zip filesystem with game resources

Here's an example of how to set up multiple filesystems:

```C++
auto root_fs = std::make_unique<CNativeFileSystem>(GetBundlePath() + "Documents/");
auto zip_fs = std::make_unique<CZipFileSystem>("Resources.zip");
auto mem_fs = std::make_unique<CMemoryFileSystem>();

root_fs->Initialize();
zip_fs->Initialize();
mem_fs->Initialize();

VirtualFileSystemPtr vfs(new VirtualFileSystem());
vfs->AddFileSystem("/", std::move(root_fs));
vfs->AddFileSystem("/resources", std::move(zip_fs));
vfs->AddFileSystem("/tmp", std::move(mem_fs));

// Example: Open a save file
if (auto saveFile = vfs->OpenFile(CFileInfo("/savefile.sav"), IFile::FileMode::Read))
{
	if (saveFile->IsOpened()) {
		// Parse game save
		// ...
	}
}

// Example: Work with a temporary file in memory
if (auto userAvatarFile = vfs->OpenFile(CFileInfo("/tmp/avatar.jpg"), IFile::FileMode::ReadWrite))
{
	if (userAvatarFile->IsOpened()) {
		// Load avatar from network and store it in memory
		// ...
		userAvatarFile->Write(data.data(), data.size());
		userAvatarFile->Close();
	}
}

// Example: Load a resource from the zip file
if (auto textureFile = vfs->OpenFile(CFileInfo("/resources/background.pvr"), IFile::FileMode::Read))
{
	if (textureFile->IsOpened()) {
		// Create texture
		// ...
	}
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

See examples/CMakeLists.txt for example of usage

## How To Build Example #

- Navigate to 'examples' directory
```bash
cd examples
```
- Run cmake to generate project
```bash
cmake -B ./build -G "Visual Studio 17 2022" .
```
