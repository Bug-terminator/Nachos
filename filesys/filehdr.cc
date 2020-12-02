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
    int i = 0, ii = 0, iii = 0; //逻辑块号
    //direct indexing
    for (; i < numSectors && i < NUMDIRECT; i++)
        if ((dataSectors[i] = freeMap->Find()) == -1)
            return FALSE;

    //single indexing
    if (numSectors > NUMDIRECT)
    {
        int buffer[SECPERIND] = {0};
        if ((dataSectors[SINGLEINDEX] = freeMap->Find()) == -1)
            return FALSE;
        for (; i < numSectors && i < NUMSINGLE; i++)
            if ((buffer[i - NUMDIRECT] = freeMap->Find()) == -1)
                return FALSE;
        synchDisk->WriteSector(dataSectors[SINGLEINDEX], (char *)buffer);
    }

    //double indexing
    if (numSectors > NUMSINGLE)
    {
        int doubleBuffer[SECPERIND] = {0};
        if ((dataSectors[DOUBLEINDEX] = freeMap->Find()) == -1)
            return FALSE;
        for (; i < numSectors && ii < SECPERIND; ++ii)
        {
            int singleBuffer[SECPERIND] = {0};
            if ((doubleBuffer[ii] = freeMap->Find()) == -1)
                return FALSE;
            for (; i < numSectors && iii < SECPERIND; i++, iii++)
                if ((singleBuffer[iii] = freeMap->Find()) == -1)
                    return FALSE;
            iii %= SECPERIND;
            synchDisk->WriteSector(doubleBuffer[ii], (char *)singleBuffer);
        }
        synchDisk->WriteSector(dataSectors[DOUBLEINDEX], (char *)doubleBuffer);
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
//终极版本
void FileHeader::Deallocate(BitMap *freeMap)
{

    int i = 0, ii, iii; //逻辑块号
    //direct indexing
    for (; i < numSectors && i < NUMDIRECT; i++)
    {
        ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
        freeMap->Clear((int)dataSectors[i]);
    }

    //single indexing
    if (numSectors > NUMDIRECT)
    {
        int buffer[SECPERIND] = {0};
        synchDisk->ReadSector(dataSectors[SINGLEINDEX], (char *)buffer);
        ASSERT(freeMap->Test((int)dataSectors[SINGLEINDEX])); // ought to be marked!
        freeMap->Clear((int)dataSectors[SINGLEINDEX]);
        for (; i < numSectors && i < NUMSINGLE; i++)
        {
            ASSERT(freeMap->Test((int)buffer[i - NUMDIRECT])); // ought to be marked!
            freeMap->Clear((int)buffer[i - NUMDIRECT]);
        }
    }

    //double indexing
    if (numSectors > NUMSINGLE)
    {
        int doubleBuffer[SECPERIND] = {0};
        synchDisk->ReadSector(dataSectors[DOUBLEINDEX], (char *)doubleBuffer);
        ASSERT(freeMap->Test((int)dataSectors[DOUBLEINDEX])); // ought to be marked!
        freeMap->Clear((int)dataSectors[DOUBLEINDEX]);
        for (ii = 0; i < numSectors && ii < SECPERIND; ++ii)
        {
            int singleBuffer[SECPERIND] = {0};
            synchDisk->ReadSector(doubleBuffer[ii], (char *)singleBuffer);
            ASSERT(freeMap->Test((int)doubleBuffer[ii])); // ought to be marked!
            freeMap->Clear((int)doubleBuffer[ii]);
            for (iii = 0; i < numSectors && iii < SECPERIND; i++, iii++)
            {
                ASSERT(freeMap->Test((int)singleBuffer[iii])); // ought to be marked!
                freeMap->Clear((int)singleBuffer[iii]);
            }
        }
    }
}
//初版
// void FileHeader::Deallocate(BitMap *freeMap)
// {

//     if (numSectors <= NUMDIRECT) //直接索引
//     {
//         for (int i = 0; i < numSectors; i++)
//         {
//             ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
//             freeMap->Clear((int)dataSectors[i]);
//         }
//     }
//     else if (numSectors <= NUMSINGLE) //一级索引
//     {
//         for (int i = 0; i < NUMDIRECT + 1; i++) //+1 for primary it self
//         {
//             ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
//             freeMap->Clear((int)dataSectors[i]);
//         }
//         int buffer[SECPERIND];
//         if (dataSectors[NUMDIRECT] < 0)
//             cout << 111111111 << endl;
//         synchDisk->ReadSector(dataSectors[NUMDIRECT], (char *)buffer);
//         for (int i = 0; i < numSectors - NUMDIRECT; ++i)
//         {
//             ASSERT(freeMap->Test((int)buffer[i])); // ought to be marked!
//             freeMap->Clear((int)buffer[i]);
//         }
//     }
//     else //二级索引
//     {
//         //直接索引表
//         for (int i = 0; i < NUMDIRECT + 2; i++)
//         {
//             // ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
//             freeMap->Clear((int)dataSectors[i]);
//         }
//         //一级索引表
//         int buffer[SECPERIND];
//         if (dataSectors[NUMDIRECT] < 0)
//             cout << 111111112 << endl;
//         synchDisk->ReadSector(dataSectors[NUMDIRECT], (char *)buffer);
//         for (int i = 0; i < SECPERIND; ++i)
//         {
//             ASSERT(freeMap->Test((int)buffer[i])); // ought to be marked!
//             freeMap->Clear((int)buffer[i]);
//         }

//         //二级索引表
//         int secBuffer[SECPERIND];
//         if (dataSectors[NUMDIRECT + 1] < 0)
//             cout << 111111113 << endl;
//         synchDisk->ReadSector(dataSectors[NUMDIRECT + 1], (char *)secBuffer);
//         for (int i = 0; i < (numSectors - NUMSINGLE - 1) / 32 + 1; ++i)
//         {

//             int firBuffer[SECPERIND];
//             if (secBuffer[i] < 0)
//                 cout << 111111114 << endl;
//             synchDisk->ReadSector(secBuffer[i], (char *)firBuffer);

//             //是否是最后一轮？
//             int limit_j = (i != (numSectors - NUMSINGLE - 1) / 32) ? SECPERIND : (numSectors - NUMSINGLE) % SECPERIND;
//             //clear 一级索引中的物理块
//             for (int j = 0; j < limit_j; ++j)
//             {
//                 ASSERT(freeMap->Test((int)firBuffer[j])); // ought to be marked!
//                 freeMap->Clear((int)firBuffer[j]);
//             }
//             //clear二级索引中的一级索引
//             ASSERT(freeMap->Test((int)secBuffer[i])); // ought to be marked!
//             freeMap->Clear((int)secBuffer[i]);
//         }
//     }
// }

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
    // inodeSector = sector;
    if (sector < 0)
        cout << 1111115 << endl;

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
    {
        if (dataSectors[secNum] < 0)
            cout << 111111111116 << endl;
        return dataSectors[secNum];
    }
    else if (secNum < NUMSINGLE) //一级索引
    {
        int buffer[SECPERIND];
        if (dataSectors[NUMDIRECT] < 0)
            cout << 1111115 << endl;
        synchDisk->ReadSector(dataSectors[NUMDIRECT], (char *)buffer);

        if (buffer[secNum - NUMDIRECT] < 0)
            cout << 111111111116 << endl;
        return buffer[secNum - NUMDIRECT];
    }
    else //二级索引
    {
        int secBuffer[SECPERIND], firBuffer[SECPERIND];
        if (dataSectors[NUMDIRECT + 1] < 0)
            cout << 1111111117 << endl;
        synchDisk->ReadSector(dataSectors[NUMDIRECT + 1], (char *)secBuffer);
        const int index = (secNum - NUMSINGLE) / SECPERIND, offset = (secNum - NUMSINGLE) % SECPERIND;
        if (secBuffer[index] < 0)
            cout << 111111111118 << endl;
        synchDisk->ReadSector(secBuffer[index], (char *)firBuffer);
        if (firBuffer[offset] < 0)
            cout << 1111111111119 << endl;
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
    if ('\040' <= c && c <= '176')
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
    // printf("----------------------------------------------\n");
    printf("File type: %s\n", enumTostirng[fileType]);
    printf("Created: %s", GetCreateTime());
    printf("Modified: %s", GetLastModifiedTime());
    printf("Visited: %s", GetLastVisitedTime());

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    //lab4 exercise3
    // printf("now in inode :%d\n", inodeSector);
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
        if (numSectors > NUMSINGLE)
        {
            printf("\n  Double indirect indexing: (mapping table sector: %d)", dataSectors[NUMDIRECT + 1]);
            synchDisk->ReadSector(dataSectors[NUMDIRECT + 1], (char *)doubleIndirectIndex);
            for (i = NUMSINGLE, ii = 0; (i < numSectors) && (ii < SECPERIND); ii++)
            {
                printf("\n    single indirect indexing: (mapping table sector: %d)\n      ", doubleIndirectIndex[ii]);
                synchDisk->ReadSector(doubleIndirectIndex[ii], (char *)singleIndirectIndex);
                for (iii = 0; (i < numSectors) && (iii < SECPERIND); i++, iii++)
                    printf("%d ", singleIndirectIndex[iii]);
            }
        }
    }

    printf("\n");
    // printf("\nFile contents:\n");
    // for (i = k = 0; (i < numSectors) && (i < NUMDIRECT); i++)
    // {
    //     synchDisk->ReadSector(dataSectors[i], data);
    //     for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
    //         printChar(data[j]);
    //     printf("\n");
    // }
    // if (numSectors > NUMDIRECT)
    // {
    //     synchDisk->ReadSector(dataSectors[NUMDIRECT], (char *)singleIndirectIndex);
    //     for (i = NUMDIRECT, ii = 0; (i < numSectors) && (ii < SECPERIND); i++, ii++)
    //     {
    //         synchDisk->ReadSector(singleIndirectIndex[ii], data);
    //         for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
    //             printChar(data[j]);
    //         printf("\n");
    //     }
    //     if (numSectors > NUMSINGLE)
    //     {
    //         synchDisk->ReadSector(dataSectors[NUMDIRECT + 1], (char *)doubleIndirectIndex);
    //         for (i = NUMSINGLE, ii = 0; (i < numSectors) && (ii < SECPERIND); ii++)
    //         {
    //             synchDisk->ReadSector(doubleIndirectIndex[ii], (char *)singleIndirectIndex);
    //             for (iii = 0; (i < numSectors) && (iii < SECPERIND); i++, iii++)
    //             {
    //                 synchDisk->ReadSector(singleIndirectIndex[iii], data);
    //                 for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
    //                     printChar(data[j]);
    //                 printf("\n");
    //             }
    //         }
    //     }
    // }
    // printf("----------------------------------------------\n");
    delete[] data;
}

//----------------------------------------------------------------------
// FileHeader::time_t到可读的时间串
//----------------------------------------------------------------------
char *FileHeader::TimeToString(time_t t)
{
    struct tm *timeinfo;
    timeinfo = localtime(&t);
    return asctime(timeinfo);
}

//----------------------------------------------------------------------
// FileHeader::扩展文件大小
//----------------------------------------------------------------------
bool FileHeader::expandFile(BitMap *freeMap, int extraCharNum)
{
    int extraBytes = extraCharNum - (numSectors * SectorSize - numBytes), extraSectors = divRoundUp(extraBytes, SectorSize);

    int i = numSectors, ii, iii;
    numBytes += extraCharNum;
    numSectors += extraSectors;
    //direct indexing
    for (; i < numSectors && i < NUMDIRECT; i++)
        if ((dataSectors[i] = freeMap->Find()) == -1)
            return FALSE;

    //single indexing
    if (numSectors > NUMDIRECT && i < NUMSINGLE)
    {
        int buffer[SECPERIND] = {0};
        if (dataSectors[SINGLEINDEX])
            synchDisk->ReadSector(dataSectors[SINGLEINDEX], (char *)buffer);
        else if ((dataSectors[SINGLEINDEX] = freeMap->Find()) == -1)
            return FALSE;
        for (; i < numSectors && i < NUMSINGLE; i++)
            if ((buffer[i - NUMDIRECT] = freeMap->Find()) == -1)
                return FALSE;
        synchDisk->WriteSector(dataSectors[SINGLEINDEX], (char *)buffer);
    }

    //double indexing
    if (numSectors > NUMSINGLE)
    {
        ii = (i - NUMSINGLE) / SECPERIND;
        iii = (i - NUMSINGLE) % SECPERIND;
        int doubleBuffer[SECPERIND] = {0};
        if (dataSectors[DOUBLEINDEX])
            synchDisk->ReadSector(dataSectors[DOUBLEINDEX], (char *)doubleBuffer);
        else if ((dataSectors[DOUBLEINDEX] = freeMap->Find()) == -1)
            return FALSE;
        for (; i < numSectors && ii < SECPERIND; ++ii)
        {
            int singleBuffer[SECPERIND] = {0};
            if (doubleBuffer[ii])
                synchDisk->ReadSector(doubleBuffer[ii], (char *)singleBuffer);
            else if ((doubleBuffer[ii] = freeMap->Find()) == -1)
                return FALSE;
            for (; i < numSectors && iii < SECPERIND; i++, iii++)
                if ((singleBuffer[iii] = freeMap->Find()) == -1)
                    return FALSE;
            iii %= SECPERIND;
            synchDisk->WriteSector(doubleBuffer[ii], (char *)singleBuffer);
        }
        synchDisk->WriteSector(dataSectors[DOUBLEINDEX], (char *)doubleBuffer);
    }
    return TRUE;
}
