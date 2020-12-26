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
#include <time.h> //lab5 excercise2

//原始版本 NumDirect = 30
// #define NumDirect ((SectorSize - 2 * sizeof(int)) / sizeof(int))
//lab5 exercise2 NumDirect = 24
#define NumDirect ((SectorSize - 8 * sizeof(int)) / sizeof(int))
#define MaxFileSize (NumDirect * SectorSize)
#define TIMELENGTH 24
//lab4 exercise3
#define SECPERIND 32                      //每一个间接索引可以表示的物理块数，sectors per indirect
#define NUMDIRECT 22                      //直接索引表示的最大块数
#define NUMSINGLE (NUMDIRECT + SECPERIND) //一级索引表达的最大块数
#define SINGLEINDEX NUMDIRECT             //一级索引下标
#define DOUBLEINDEX (SINGLEINDEX + 1)     //二级索引下标
typedef enum
{
  NORM,
  DIR
} FileType;
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
  int FileSecoters();
  void Print(); // Print the contents of the file.

  //lab4 exercise2
  void SetLastVisitedTime(void) { time(&lastVisitedTime); }
  char *GetLastVisitedTime(void) { return TimeToString(lastVisitedTime); }
  void SetLastModifiedTime(void) { time(&lastModifiedTime); }
  char *GetLastModifiedTime(void) { return TimeToString(lastModifiedTime); }
  void SetCreateTime(void) { time(&createTime); }
  char *GetCreateTime(void) { return TimeToString(createTime); }
  void SetPath(char *pth) { path = pth; }
  char *GetPath(void) { return path; }
  void SetFileType(FileType tp) { fileType = tp; }
  FileType GetFileType() { return fileType; }

  // 在openfile.cc析构函数中writeBack()，以保存各种时间信息
  void SetInodeSector(int sector) { inodeSector = sector; }
  int GetInodeSector() { return inodeSector; }
  //lab4 exercise5
  bool expandFile(BitMap *freeMap, int extraCharNum);

private:
  int numBytes;               // Number of bytes in the file
  int numSectors;             // Number of data sectors in the file
  int dataSectors[NumDirect]; // Disk sector numbers for each data
                              // block in the file

  //lab4 exercise2
  time_t lastVisitedTime;
  time_t lastModifiedTime;
  time_t createTime;
  char *path;
  FileType fileType;
  int inodeSector; //openfile析构时需要保存信息
  char *TimeToString(time_t t);

  // lab4 exercise5
  // int stPtr[3];//start ptr,分别指向直接/一级/二级索引开始的位置，用于文件大小的扩展
};

#endif // FILEHDR_H
