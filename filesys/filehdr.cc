// filehdr.cc
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector,
//
//      Unlike in a real system, we do not keep track of file permissions,
//	ownership, last modification date, etc., in the file header.
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "filehdr.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------
//----------------------------------------------------------------------
// lab4 突破文件长度限制
// 根据文件长度分配内存需
// 要分为两步：先分配直接
// 索引，再分配一级索引。
//----------------------------------------------------------------------

bool FileHeader::Allocate(BitMap *freeMap, int fileSize, int tp)
{
    type = tp;
    numBytes = fileSize;
    numSectors = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors)
    {

        DEBUG('d', "There is not enough space.\tfile:%s\tline:%d\n", __FILE__, __LINE__); //d means disk
        return FALSE;                                                                     // not enough space
    }
    //直接索引
    if (numSectors <= DIRECT_NUM)
    {
        // DEBUG('d', "Using direct allocation.\n");
        for (int i = 0; i < numSectors; i++)
            dataSectors[i] = freeMap->Find();
    }
    //一级索引
    else if (numSectors <= DIRECT_NUM + PRIMARY_NUM)
    {
        // DEBUG('d', "Using indirect allocation.\n");
        //直接索引
        for (int i = 0; i < DIRECT_NUM; i++)
            dataSectors[i] = freeMap->Find();
        //一级索引
        int primary[PRIMARY_NUM];
        //将物理块号存入一级索引表中
        for (int i = 0; i < numSectors - DIRECT_NUM; ++i)
            primary[i] = freeMap->Find();
        //为一级索引表分配物理空间
        int numSector = freeMap->Find();
        if (numSector == -1)
        {
            DEBUG('d', "No space for primary table.\tfile:%s\tline:%d\n", __FILE__, __LINE__);
            return FALSE;
        }
        //将一级索引表表号写入dataSectors中
        dataSectors[DIRECT_NUM] = numSector;
        //将一级索引表写入磁盘
        synchDisk->WriteSector(numSector, (char *)primary);
    }
    //文件长度超过限制
    else
    {
        DEBUG('d', "File length exceed!\tfile:%s\tline:%d\n", __FILE__, __LINE__);
        return FALSE;
    }
    createTime = stats->totalTicks; //lab4 文件创造时间
    return TRUE;

    // 原始版本
    // numBytes = fileSize;
    // numSectors = divRoundUp(fileSize, SectorSize);
    // if (freeMap->NumClear() < numSectors)
    //     return FALSE; // not enough space

    // for (int i = 0; i < numSectors; i++)
    //     dataSectors[i] = freeMap->Find();
    // createTime = stats->totalTicks; //lab4 文件创造时间
    // return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void FileHeader::Deallocate(BitMap *freeMap)
{
    //直接索引
    if (numSectors <= DIRECT_NUM)
    {
        for (int i = 0; i < numSectors; i++)
        {
            ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
            freeMap->Clear((int)dataSectors[i]);
        }
    }
    //一级索引
    else if (numSectors <= DIRECT_NUM + PRIMARY_NUM)
    {
        //直接索引
        for (int i = 0; i < DIRECT_NUM; i++)
        {
            ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
            freeMap->Clear((int)dataSectors[i]);
        }
        //在内存中初始化一个一级索引数组
        int primary[PRIMARY_NUM];
        //取出一级索引表表号
        int numSector = dataSectors[DIRECT_NUM];
        //将一级索引表读入内存
        synchDisk->ReadSector(numSector, (char *)primary);
        //清除一级索引表的磁盘空间
        ASSERT(freeMap->Test((int)numSector)); // ought to be marked!
        freeMap->Clear((int)numSector);
        //释放一级索引表中的空间
        for (int i = 0; i < numSectors - DIRECT_NUM; ++i)
        {
            ASSERT(freeMap->Test((int)primary[i])); // ought to be marked!
            freeMap->Clear((int)primary[i]);
        }
    }
    // 原始版本
    // for (int i = 0; i < numSectors; i++)
    // {
    //     ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
    //     freeMap->Clear((int)dataSectors[i]);
    // }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------
int FileHeader::ByteToSector(int offset)
{
    //lab4 新增上次访问时间
    lastVisitedTime = stats->totalTicks;
    //lab4 改为间接索引
    int v_index = offset / SectorSize; //虚拟块号
    //虚拟索引号不能大于直接索引+间接索引的总和
    ASSERT(v_index < DIRECT_NUM + PRIMARY_NUM);
    //直接索引
    if (v_index < DIRECT_NUM)
    {
        return dataSectors[v_index];
    }
    else
    {
        //在内存中初始化一个一级索引数组
        int primary[PRIMARY_NUM];
        //取出一级索引表表号
        int numSector = dataSectors[DIRECT_NUM];
        //将一级索引表读入内存
        synchDisk->ReadSector(numSector, (char *)primary);
        return primary[v_index - DIRECT_NUM];
    }
    //原始版本
    // lastVisitedTime = stats->totalTicks; //lab4 新增上次访问时间
    // return (dataSectors[offset / SectorSize]);
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numSectors; i++)
        printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++)
    {
        synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
        {
            if ('\040' <= data[j] && data[j] <= '\176') // isprint(data[j])
                printf("%c", data[j]);
            else
                printf("\\%x", (unsigned char)data[j]);
        }
        printf("\n");
    }
    delete[] data;
}
