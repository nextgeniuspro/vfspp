# vfspp

vfspp - C++ Virtual File System that allow to manipulate with files from memory, zip archives or native filesystems like it is just native filesystem. It is useful for game developers to use resources in native filesystem during development and then pack their to the archive in distribution build. 

```
// Register native filesystem during developnment or zip for distribution build
IFileSystemPtr root_fs = nullptr;
#if !defined(DISTRIBUTION_BUILD)
root_fs.reset(new CNativeFileSystem(GetBundlePath() + "resources/"));
#else
root_fs.reset(new CZipFileSystem("Resources.zip", "/"));
#endif

root_fs->Initialize();
vfs_get_global()->AddFileSystem("/", root_fs);
```

Sometimes useful to have several mounted filesystem, like "/" - is you root native filesystem, "/tmp" - memory filesystem, which allow to work with temporary files without disk operations, "/resources" - zip filesystem with game resources.

```
std::string zipPassword = "123";

FileSystemPtr root_fs(new CNativeFileSystem(GetBundlePath() + "Documents/"));
IFileSystemPtr zip_fs(new CZipFileSystem("Resources.zip", "/", false, zipPassword));
IFileSystemPtr mem_fs(new CMemoryFileSystem());

root_fs->Initialize();
zip_fs->Initialize();
mem_fs->Initialize();

CVirtualFileSystemPtr vfs = vfs_get_global();
vfs->AddFileSystem("/", root_fs);
vfs->AddFileSystem("/resources", zip_fs);
vfs->AddFileSystem("/tmp", mem_fs);

IFilePtr saveFile = vfs->OpenFile(CFileInfo("/savefile.sav"), IFile::In);
if (saveFile && saveFile->IsOpened())
{
    // Parse game save
}

IFilePtr userAvatarFile = vfs->OpenFile(CFileInfo("/tmp/avatar.jpg"), IFile::ReadWrite);
if (userAvatarFile && userAvatarFile->IsOpened())
{
	// Load avatar from network and store it in memory
	userAvatarFile->Write(data, data.size());
	userAvatarFile->Close();
}

IFilePtr textureFile = vfs->OpenFile(CFileInfo("/resources/background.pvr"), IFile::In);
if (textureFile && textureFile->IsOpened())
{
	// Create texture
}
```

# How To Build #

```
cmake .
make
```

Use CMAKE_INSTALL_PREFIX to change instalation directory

```
cmake -DCMAKE_INSTALL_PREFIX=/usr
make
make install
```

To generate project file use -G option

```
cmake -G Xcode .
or
cmake -G "Visual Studio 14 2015 Win64" .
```

Add parameter WITH_EXAMPLES to build with example project

```
cmake -G Xcode . -DWITH_EXAMPLES=1
```