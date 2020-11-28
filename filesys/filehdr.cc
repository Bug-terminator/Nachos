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
//  lab4 突破文件长度限制
//  根据文件长度分配内存需
//  要分为两步：先分配直接
//  索引，再分配一级索引,
//  最后分配二级索引。
//----------------------------------------------------------------------
bool FileHeader::Allocate(BitMap *freeMap, int fileSize)
{
    numBytes = fileSize;
    numSectors = divRoundUp(fileSize, SectorSize);
    if (numSectors <= NUMDIRECT) //直接索引
    {
        if (freeMap->NumClear() < numSectors)
            return FALSE; // not enough space

        for (int i = 0; i < numSectors; i++)
            dataSectors[i] = freeMap->Find();
    }
    else if (numSectors <= NUMFIRST) //一级索引
    {
        if (freeMap->NumClear() < numSectors + 1) //+1 是因为一级索引
            return FALSE;
        for (int i = 0; i < NUMDIRECT; i++)
            dataSectors[i] = freeMap->Find();
        int sector = freeMap->Find(); //for primary index
        int buffer[SECPERIND];
        for (int i = 0; i < numSectors - NUMDIRECT; ++i)
            buffer[i] = freeMap->Find();
        dataSectors[NUMDIRECT] = sector;
        synchDisk->WriteSector(sector, (char *)buffer);
    }
    else //二级索引
    {
        if (freeMap->NumClear() < numSectors + 1 + ((numSectors - NUMFIRST - 1) / 32 + 1) + 1) //(numSectors - NUMFIRST - 1)/32 + 1表示
            return FALSE;                                                                      //二級索引中的一級索引，最后的+1为二级索引
        for (int i = 0; i < NUMDIRECT; i++)
            dataSectors[i] = freeMap->Find();

        int sector = freeMap->Find(); //for primary index
        int buffer[SECPERIND];
        for (int i = 0; i < SECPERIND; ++i)
            buffer[i] = freeMap->Find();
        dataSectors[NUMDIRECT] = sector;
        synchDisk->WriteSector(sector, (char *)buffer);

        int sector2 = freeMap->Find(); //for secondary index
        dataSectors[NUMDIRECT + 1] = sector2;
        int secBuffer[SECPERIND];
        for (int i = 0; i < (numSectors - NUMFIRST - 1) / 32 + 1; ++i)
        {
            sector = freeMap->Find();
            secBuffer[i] = sector;
            int firBuffer[SECPERIND];
            //是否是最后一轮？
            int limit_j = (i == (numSectors - NUMFIRST - 1) / 32) ? SECPERIND : (numSectors - NUMFIRST) % SECPERIND;
            for (int j = 0; j < limit_j; ++j)
                firBuffer[j] = freeMap->Find();
            synchDisk->WriteSector(sector, (char *)firBuffer);
        }
        synchDisk->WriteSector(sector2, (char *)secBuffer);
    }

    //lab4 exercise2
    SetCreateTime();
    SetLastModifiedTime();
    SetLastVisitedTime();
    return TRUE;
}

//原始版本
// bool FileHeader::Allocate(BitMap *freeMap, int fileSize)
// {
//     numBytes = fileSize;
//     numSectors = divRoundUp(fileSize, SectorSize);
//     if (freeMap->NumClear() < numSectors)
//         return FALSE; // not enough space

//     for (int i = 0; i < numSectors; i++)
//         dataSectors[i] = freeMap->Find();
//     //lab4 exercise2
//     SetCreateTime();
//     SetLastModifiedTime();
//     SetLastVisitedTime();
//     return TRUE;
// }

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void FileHeader::Deallocate(BitMap *freeMap)
{

    if (numSectors <= NUMDIRECT) //直接索引
    {
        for (int i = 0; i < numSectors; i++)
        {
            ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
            freeMap->Clear((int)dataSectors[i]);
        }
    }
    else if (numSectors <= NUMFIRST) //一级索引
    {
        for (int i = 0; i < NUMDIRECT + 1; i++) //+1 for primary it self
        {
            ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
            freeMap->Clear((int)dataSectors[i]);
        }

        int buffer[SECPERIND];
        synchDisk->ReadSector(dataSectors[NUMDIRECT], (char *)buffer);
        for (int i = 0; i < numSectors - NUMDIRECT; ++i)
        {
            ASSERT(freeMap->Test((int)buffer[i])); // ought to be marked!
            freeMap->Clear((int)buffer[i]);
        }
    }
    else //二级索引
    {
        //直接索引表
        for (int i = 0; i < NUMDIRECT + 2; i++)
        {
            ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
            freeMap->Clear((int)dataSectors[i]);
        }

        //一级索引表
        int buffer[SECPERIND];
        synchDisk->ReadSector(dataSectors[NUMDIRECT], (char *)buffer);
        for (int i = 0; i < SECPERIND; ++i)
        {
            ASSERT(freeMap->Test((int)buffer[i])); // ought to be marked!
            freeMap->Clear((int)buffer[i]);
        }

        //二级索引表
        int secBuffer[SECPERIND];
        synchDisk->ReadSector(dataSectors[NUMDIRECT + 1], (char *)secBuffer);
        for (int i = 0; i < (numSectors - NUMFIRST - 1) / 32 + 1; ++i)
        {

            int firBuffer[SECPERIND];
            synchDisk->ReadSector(secBuffer[i], (char *)firBuffer);

            //是否是最后一轮？
            int limit_j = (i == (numSectors - NUMFIRST - 1) / 32) ? SECPERIND : (numSectors - NUMFIRST) % SECPERIND;
            //clear 一级索引中的物理块
            for (int j = 0; j < limit_j; ++j)
            {
                ASSERT(freeMap->Test((int)firBuffer[i])); // ought to be marked!
                freeMap->Clear((int)firBuffer[i]);
            }
            //clear二级索引中的一级索引
            ASSERT(freeMap->Test((int)secBuffer[i])); // ought to be marked!
            freeMap->Clear((int)secBuffer[i]);
        }
    }
}

//原始版本
// void FileHeader::Deallocate(BitMap *freeMap)
// {
//     for (int i = 0; i < numSectors; i++)
//     {
//         ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
//         freeMap->Clear((int)dataSectors[i]);
//     }
// }

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void FileHeader::FetchFrom(int sector)
{
    //在这里获取inodeSector
    inodeSector = sector;
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
    int secNum = offset / SectorSize;
    if (secNum < NUMDIRECT) //直接索引
        return dataSectors[secNum];
    else if (secNum < NUMFIRST) //一级索引
    {
        int buffer[SECPERIND];
        synchDisk->ReadSector(dataSectors[NUMDIRECT], (char *)buffer);
        return buffer[secNum - NumDirect];
    }
    else //二级索引
    {
        int secBuffer[SECPERIND], firBuffer[SECPERIND];
        synchDisk->ReadSector(dataSectors[NUMDIRECT + 1], (char *)secBuffer);
        const int index = (secNum - NUMFIRST) / SECPERIND, offset = (secNum - NUMFIRST) % SECPERIND;
        synchDisk->ReadSector(secBuffer[index], (char *)firBuffer);
        return firBuffer[offset];
    }
}
//原始版本
// int FileHeader::ByteToSector(int offset)
// {
//     return (dataSectors[offset / SectorSize]);
// }

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// printChar
//----------------------------------------------------------------------
void printChar(char c)
{
    if('\040' <= c && c <= '176')
    printf("%c", c);
    else
    printf("\\%x", (unsigned char)c);
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------
const char *enumTostirng[2] = {"NORM", "DIR"}; //用于enum到string的转换，debug用
void FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    //lab4 exercise2
    printf("----------------------------------------------\n");
    printf("File type: %s\n", enumTostirng[fileType]);
    printf("Created: %s", createTime);
    printf("Modified: %s", lastModifiedTime);
    printf("Visited: %s", lastVisitedTime);

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numSectors; i++)
        printf("%d ", dataSectors[i]);

    //lab4 exercise3
    int ii, iii;                        // For single / double indirect indexing
    int singleIndirectIndex[SECPERIND]; // used to restore the indexing map
    int doubleIndirectIndex[SECPERIND]; // used to restore the indexing map
    printf("  Direct indexing:\n    ");
    for (i = 0; (i < numSectors) && (i < NUMDIRECT); i++)
        printf("%d ", dataSectors[i]);
    if (numSectors > NUMDIRECT)
    {
        printf("\n  Indirect indexing: (mapping table sector: %d)\n    ", dataSectors[NUMDIRECT]);
        synchDisk->ReadSector(dataSectors[NUMDIRECT], (char *)singleIndirectIndex);
        for (i = NumDirect, ii = 0; (i < numSectors) && (ii < SECPERIND); i++, ii++)
            printf("%d ", singleIndirectIndex[ii]);
        if (numSectors > NUMFIRST)
        {
            printf("\n  Double indirect indexing: (mapping table sector: %d)", dataSectors[NUMDIRECT + 1]);
            synchDisk->ReadSector(dataSectors[NUMDIRECT + 1], (char *)doubleIndirectIndex);
            for (i = NUMDIRECT + SECPERIND, ii = 0; (i < numSectors) && (ii < SECPERIND); ii++)
            {
                printf("\n    single indirect indexing: (mapping table sector: %d)\n      ", doubleIndirectIndex[ii]);
                synchDisk->ReadSector(doubleIndirectIndex[ii], (char *)singleIndirectIndex);
                for (iii = 0; (i < numSectors) && (iii < SECPERIND); i++, iii++)
                    printf("%d ", singleIndirectIndex[iii]);
            }
        }
    }
    
    printf("\nFile contents:\n");
    for (i = k = 0; (i < numSectors) && (i < NUMDIRECT); i++)
    {
        synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
            printChar(data[j]);
        printf("\n");
    }
    if (numSectors > NUMDIRECT) {
        synchDisk->ReadSector(dataSectors[NUMDIRECT], (char*)singleIndirectIndex);
        for (i = NUMDIRECT, ii = 0; (i < numSectors) && (ii < SECPERIND); i++, ii++) {
            synchDisk->ReadSector(singleIndirectIndex[ii], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
                printChar(data[j]);
            printf("\n");
        }
        if (numSectors > NUMFIRST) {
            synchDisk->ReadSector(dataSectors[NUMDIRECT+1], (char*)doubleIndirectIndex);
            for (i = NUMFIRST, ii = 0; (i < numSectors) && (ii < SECPERIND); ii++) {
                synchDisk->ReadSector(doubleIndirectIndex[ii], (char*)singleIndirectIndex);
                for (iii = 0; (i < numSectors) && (iii < SECPERIND); i++, iii++) {
                    synchDisk->ReadSector(singleIndirectIndex[iii], data);
                    for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
                        printChar(data[j]);
                    printf("\n");
                }
            }
        }
    }
    printf("----------------------------------------------\n");
    delete[] data;
}

//----------------------------------------------------------------------
// FileHeader::GetCurrentTime
//----------------------------------------------------------------------
char *FileHeader::GetCurrentTime()
{
    time_t t;
    struct tm *timeinfo;
    time(&t);
    timeinfo = localtime(&t);
    return asctime(timeinfo);
}