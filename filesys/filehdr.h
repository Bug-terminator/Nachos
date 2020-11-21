// filehdr.h
//	Data structures for managing a disk file header.
//
//	A file header describes where on disk to find the data in a file,
//	along with other information about the file (for instance, its
//	length, owner, etc.)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef FILEHDR_H
#define FILEHDR_H

#include "disk.h"
#include "bitmap.h"

//----------------------------------------------------------------------
//Lab4 Exercise2 新增成员变量
//----------------------------------------------------------------------
#define VAR_NUM 7                                                      //i-node中的变量数，变量的均占4B(int, char *), i-node中共7个变量，所以为7
#define NumDirect ((SectorSize - VAR_NUM * sizeof(int)) / sizeof(int)) // i-node中索引表大小,值为 \
                                                                       //（128 -  4*7）/4 = 25
#define MaxFileSize (NumDirect * SectorSize)                           //文件最大长度 25 * 128 = 3200

//原始版本
// #define NumDirect ((SectorSize - 2 * sizeof(int)) / sizeof(int)) // inode中索引表大小
// #define MaxFileSize (NumDirect * SectorSize) //文件最大长度

//----------------------------------------------------------------------
//Lab4 Exercise3 改为间接索引
//----------------------------------------------------------------------
#define DIRECT_NUM 24  //直接索引数，表示[0,24)块
#define PRIMARY_NUM (SectorSize / sizeof(int)) //一级索引块数，表示[24,56)块。文件最大长度 = 24 * 128 + 128/4 * 128 =  7168满足题意
// #define SECONDARY_INDEX //二级索引（if needed）
// #define DIRECT_MAX DIRECT_INDEX *SectorSize                              //直接索引的地址范围[0~3072）
// #define PRIMARY_MAX (DIRECT_MAX + SectorSize / sizeof(int) * SectorSize) //一级索引的地址范围[3072,7168)

// The following class defines the Nachos "file header" (in UNIX terms,
// the "i-node"), describing where on disk to find all of the data in the file.
// The file header is organized as a simple table of pointers to
// data blocks.
//
// The file header data structure can be stored in memory or on disk.
// When it is on disk, it is stored in a single sector -- this means
// that we assume the size of this data structure to be the same
// as one disk sector.  Without indirect addressing, this
// limits the maximum file length to just under 4K bytes.
//
// There is no constructor; rather the file header can be initialized
// by allocating blocks for the file (if it is a new file), or by
// reading it from disk.

class FileHeader
{
public:
  //lab4新增
  int createTime;
  int lastVisitedTime;
  int lastModifiedTime;
  int type;
  char *path;

  bool Allocate(BitMap *bitMap, int fileSize); // Initialize a file header,
                                               //  including allocating space
                                               //  on disk for the file data
  void Deallocate(BitMap *bitMap);             // De-allocate this file's
                                               //  data blocks

  void FetchFrom(int sectorNumber); // Initialize file header from disk
  void WriteBack(int sectorNumber); // Write modifications to file header
                                    //  back to disk

  int ByteToSector(int offset); // Convert a byte offset into the file
                                // to the disk sector containing
                                // the byte

  int FileLength(); // Return the length of the file
                    // in bytes

  void Print(); // Print the contents of the file.

private:
  //原始版本
  int numBytes;               // Number of bytes in the file
  int numSectors;             // Number of data sectors in the file
  int dataSectors[NumDirect]; // Disk sector numbers for each data
                              // block in the file
};

#endif // FILEHDR_H
