// filesys.cc
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "disk.h"
#include "bitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known
// sectors, so that they can be located on boot-up.
#define FreeMapSector 0
#define DirectorySector 1

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number
// of files that can be loaded onto the disk.
#define FreeMapFileSize (NumSectors / BitsInByte)
#define NumDirEntries 10
#define DirectoryFileSize (sizeof(DirectoryEntry) * NumDirEntries)

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format)
{
    DEBUG('f', "Initializing the file system.\n");
    if (format)
    {
        BitMap *freeMap = new BitMap(NumSectors);
        Directory *directory = new Directory(NumDirEntries);
        FileHeader *mapHdr = new FileHeader;
        FileHeader *dirHdr = new FileHeader;

        DEBUG('f', "Formatting the file system.\n");

        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        freeMap->Mark(FreeMapSector);
        freeMap->Mark(DirectorySector);

        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!

        ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize, NORM));
        ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize, DIR));

        // Flush the bitmap and directory FileHeaders back to disk
        // We need to do this before we can "Open" the file, since open
        // reads the file header off of disk (and currently the disk has garbage
        // on it!).

        DEBUG('f', "Writing headers back to disk.\n");
        mapHdr->WriteBack(FreeMapSector);
        dirHdr->WriteBack(DirectorySector);

        // OK to open the bitmap and directory files now
        // The file system operations assume these two files are left open
        // while Nachos is running.

        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);

        // Once we have the files "open", we can write the initial version
        // of each file back to disk.  The directory at this point is completely
        // empty; but the bitmap has been changed to reflect the fact that
        // sectors on the disk have been allocated for the file headers and
        // to hold the file data for the directory and bitmap.

        DEBUG('f', "Writing bitmap and directory back to disk.\n");
        freeMap->WriteBack(freeMapFile); // flush changes to disk
        directory->WriteBack(directoryFile);

        if (DebugIsEnabled('f'))
        {
            freeMap->Print();
            directory->Print();

            delete freeMap;
            delete directory;
            delete mapHdr;
            delete dirHdr;
        }
    }
    else
    {
        // if we are not formatting the disk, just open the files representing
        // the bitmap and directory; these are left open while Nachos is running
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
    }
}

//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------
//lab4 多级目录
bool FileSystem::Create(char *path, int dirInode, int initialSize, BitMap *btmp)
{
    //根据path划分name
    bool self = false, root = false; //该文件是否是文件本身？是否是根目录？
    char *name = path, *p = path;
    if (path[0] == '/') //根目录
    {
        name = path + 1;
        root = true;
    }
    while (*p != '/' && *p != '\0')
        p++;
    if (*p == '\0')
        self = true;
    else
        *p = '\0';

    //准备工作
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *dirFile = NULL;

    if (root) //将目录读入内存
        directory->FetchFrom(directoryFile);
    else
    {
        dirFile = new OpenFile(dirInode);
        //断言该文件为目录文件，防止错误地在普通文件中创建新文件
        ASSERT(dirFile->getInode()->type == DIR);
        directory->FetchFrom(dirFile);
    }

    BitMap *freeMap;
    if (root) //根节点，需要从磁盘读入位图
    {
        freeMap = new BitMap(NumSectors);
        freeMap->FetchFrom(freeMapFile);
    }
    else //否则用内存中的位图
        freeMap = btmp;
    bool success;
    int sector;
    DEBUG('f', "Creating file %s, size %d\n", path, initialSize);

    //文件本身，递归出口
    if (self)
    {
        //已经存在
        if (directory->Find(name) != -1)
            success = FALSE;
        else
        {
            sector = freeMap->Find();
            if (sector == -1)
                success = FALSE;

            //查dir，找到空闲项，将新的inode插入
            else if (!directory->Add(name, sector))
                success = FALSE;
            else
            {
                //构造新的i-node，并分配初始化inode
                FileHeader *hdr = new FileHeader;
                if (!hdr->Allocate(freeMap, initialSize, NORM))
                    success = FALSE; // no space on disk for data
                else
                {
                    success = TRUE;
                    // 将inode写回磁盘
                    hdr->WriteBack(sector);
                    //更新磁盘中的目录和bitmap
                    if (root) //根节点，写入磁盘根目录
                        directory->WriteBack(directoryFile);
                    else //否则，写入磁盘的其他目录
                        directory->WriteBack(dirFile);
                    //唯一一次更新磁盘中bitmap的机会
                    freeMap->WriteBack(freeMapFile);
                }
                delete hdr;
            }
        }
    }
    //目录文件
    else
    {
        int nextDirInode = directory->Find(name);
        //目录已经存在,直接递归构造下一级目录
        if (nextDirInode != -1)
            success = Create(p + 1, nextDirInode, initialSize, freeMap);
        //目录尚未存在，创造一个新的目录inode
        else
        {
            sector = freeMap->Find();
            if (sector == -1)
                success = FALSE;

            //查dir，找到空闲项，将新的inode插入
            else if (!directory->Add(name, sector))
                success = FALSE;
            else
            {
                //构造新的i-node，并分配初始化inode
                FileHeader *hdr = new FileHeader;
                if (!hdr->Allocate(freeMap, DirectoryFileSize, DIR))
                    success = FALSE; // no space on disk for data
                else
                {
                    // 将inode写回磁盘
                    hdr->WriteBack(sector);
                    success = Create(p + 1, sector, initialSize, freeMap);
                    //下一级目录的物理空间成功分配
                    if (success)
                    {
                        //更新磁盘中的目录
                        if (root) //根节点，写入磁盘根目录
                            directory->WriteBack(directoryFile);
                        else //否则，写入磁盘的其他目录
                            directory->WriteBack(dirFile);
                    }
                }
                delete hdr;
            }
        }
    }
    if (root)
        delete freeMap;
    delete directory;
    if (dirFile)
        delete dirFile;
    return success;
}

// 原始版本
bool FileSystem::Create(char *name, int initialSize)
{
    Directory *directory;
    BitMap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;

    DEBUG('f', "Creating file %s, size %d\n", name, initialSize);

    //将根目录读入内存
    directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);

    if (directory->Find(name) != -1)
        success = FALSE;
    else
    {
        freeMap = new BitMap(NumSectors);
        freeMap->FetchFrom(freeMapFile);
        sector = freeMap->Find();
        if (sector == -1)
            success = FALSE;

        //查dir，找到空闲项，将新的inode插入，我感觉这里有bug，存在这样一种情况：
        //先将新的dirEntry插入了dir，然后在下面的inode初始化中失败了，这样就多了一个无用的表项。
        //11:21更新，不会，因为出错时不会writeback，磁盘中的信息没变。
        else if (!directory->Add(name, sector))
            success = FALSE;
        else
        {
            //构造新的i-node，并分配初始化inode
            hdr = new FileHeader;
            if (!hdr->Allocate(freeMap, initialSize, NORM))
                success = FALSE; // no space on disk for data
            else
            {
                success = TRUE;

                // 将inode写回磁盘
                hdr->WriteBack(sector);

                //更新磁盘中的目录和bitmap
                directory->WriteBack(directoryFile);
                freeMap->WriteBack(freeMapFile);
            }
            delete hdr;
        }
        delete freeMap;
    }
    delete directory;
    return success;
}

//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.
//	To open a file:
//	  Find the location of the file's header, using the directory
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------
//Lab4 多级目录
OpenFile *FileSystem::Open(char *path, int dirInode)
{
    //根据path划分name
    bool self = false, root = false; //该文件是否是文件本身？是否是根目录？
    char *name = path, *p = path;
    if (path[0] == '/') //根目录
    {
        name = path + 1;
        root = true;
    }
    while (*p != '/' && *p != '\0')
        p++;
    if (*p == '\0')
        self = true;
    else
        *p = '\0';

    //准备工作
    int sector;
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *dirFile = NULL, *openFile = NULL;
    if (root) //将目录读入内存
        directory->FetchFrom(directoryFile);
    else
    {
        dirFile = new OpenFile(dirInode);
        directory->FetchFrom(dirFile);
    }


    if (self) //文件本身，递归出口
    {
        DEBUG('f', "Opening file %s\n", name);
        sector = directory->Find(name);
        if (sector >= 0) //找到文件
            openFile = new OpenFile(sector);
        else
            DEBUG('f', "File doesn't exist, %s\n", name);
    }
    else //目录文件
    {
        DEBUG('f', "Opening dir %s\n", name);
        sector = directory->Find(name);
        if (sector >= 0) //找到目录，递归访问
            openFile = Open(p + 1, sector);
        else
            DEBUG('f', "Dir doesn't exist, %s\n", name);
    }
    if (dirFile)
        delete dirFile;
    delete directory;
    return openFile; // return NULL if not found}
}

//原始版本
OpenFile *
FileSystem::Open(char *name)
{
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *openFile = NULL;
    int sector;

    DEBUG('f', "Opening file %s\n", name);
    directory->FetchFrom(directoryFile);
    sector = directory->Find(name);
    if (sector >= 0)
        openFile = new OpenFile(sector); // name was found in directory
    delete directory;
    return openFile; // return NULL if not found
}

//----------------------------------------------------------------------
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------
//lab4 多级目录
bool FileSystem::Remove(char *path, int dirInode, BitMap *btmp)
{
    //根据path划分name
    bool self = false, root = false; //该文件是否是文件本身？是否是根目录？
    char *name = path, *p = path;
    if (path[0] == '/') //根目录
    {
        name = path + 1;
        root = true;
    }
    while (*p != '/' && *p != '\0')
        p++;
    if (*p == '\0')
        self = true;
    else
        *p = '\0';

    //准备工作
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *dirFile = NULL;
    if (root) //根节点，将目录读入内存
        directory->FetchFrom(directoryFile);
    else
    {
        dirFile = new OpenFile(dirInode);
        directory->FetchFrom(dirFile);
    }

    BitMap *freeMap;
    if (root) //根节点，需要从磁盘读入位图
    {
        freeMap = new BitMap(NumSectors);
        freeMap->FetchFrom(freeMapFile);
    }
    else //否则用内存中的位图
        freeMap = btmp;
    bool success;
    int sector;
    FileHeader *fileHdr;

    if (self) //文件本身,递归出口. todo:递归删除其子目录
    {
        sector = directory->Find(name);
        //文件不存在
        if (sector == -1)
        {
            DEBUG('f', "File doesn't exist. %s\n", name);
            success = FALSE; // file not found
        }
        //文件存在
        else
        {
            fileHdr = new FileHeader;
            fileHdr->FetchFrom(sector);
            fileHdr->Deallocate(freeMap); // remove data blocks
            freeMap->Clear(sector);       // remove header block
            directory->Remove(name);
            freeMap->WriteBack(freeMapFile); // flush to disk
            if (root)
                directory->WriteBack(directoryFile); // flush to disk
            else
                directory->WriteBack(dirFile);
            delete fileHdr;
            success = TRUE;
        }
    }
    //目录文件
    else
    {
        sector = directory->Find(name);
        if (sector == -1)//目录不存在
        {
            DEBUG('f', "Dir doesn't exist. %s\n", name);
            success = FALSE; // dir not found
        }
        else//递归到下一级目录
            success = Remove(p + 1, sector, freeMap);
    }
    
    delete directory;
    if(root)
    delete freeMap;
    if(dirFile)
    delete dirFile;
    return success;
}

//原始版本
bool FileSystem::Remove(char *name)
{
    Directory *directory;
    BitMap *freeMap;
    FileHeader *fileHdr;
    int sector;

    directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);
    sector = directory->Find(name);
    if (sector == -1)
    {
        delete directory;
        return FALSE; // file not found
    }
    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);

    freeMap = new BitMap(NumSectors);
    freeMap->FetchFrom(freeMapFile);

    fileHdr->Deallocate(freeMap); // remove data blocks
    freeMap->Clear(sector);       // remove header block
    directory->Remove(name);

    freeMap->WriteBack(freeMapFile);     // flush to disk
    directory->WriteBack(directoryFile); // flush to disk
    delete fileHdr;
    delete directory;
    delete freeMap;
    return TRUE;
}

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void FileSystem::List()
{
    Directory *directory = new Directory(NumDirEntries);

    directory->FetchFrom(directoryFile);
    directory->List();
    delete directory;
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void FileSystem::Print()
{
    FileHeader *bitHdr = new FileHeader;
    FileHeader *dirHdr = new FileHeader;
    BitMap *freeMap = new BitMap(NumSectors);
    Directory *directory = new Directory(NumDirEntries);

    printf("Bit map file header:\n");
    bitHdr->FetchFrom(FreeMapSector);
    bitHdr->Print();

    printf("Directory file header:\n");
    dirHdr->FetchFrom(DirectorySector);
    dirHdr->Print();

    freeMap->FetchFrom(freeMapFile);
    freeMap->Print();

    directory->FetchFrom(directoryFile);
    directory->Print();

    delete bitHdr;
    delete dirHdr;
    delete freeMap;
    delete directory;
}
