// synchdisk.cc
//	Routines to synchronously access the disk.  The physical disk
//	is an asynchronous device (disk requests return immediately, and
//	an interrupt happens later on).  This is a layer on top of
//	the disk providing a synchronous interface (requests wait until
//	the request completes).
//
//	Use a semaphore to synchronize the interrupt handlers with the
//	pending requests.  And, because the physical disk can only
//	handle one operation at a time, use a lock to enforce mutual
//	exclusion.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synchdisk.h"

//----------------------------------------------------------------------
// DiskRequestDone
// 	Disk interrupt handler.  Need this to be a C routine, because
//	C++ can't handle pointers to member functions.
//----------------------------------------------------------------------

static void
DiskRequestDone(int arg)
{
    SynchDisk *disk = (SynchDisk *)arg;

    disk->RequestDone();
}

//----------------------------------------------------------------------
// SynchDisk::SynchDisk
// 	Initialize the synchronous interface to the physical disk, in turn
//	initializing the physical disk.
//
//	"name" -- UNIX file name to be used as storage for the disk data
//	   (usually, "DISK")
//----------------------------------------------------------------------

SynchDisk::SynchDisk(char *name)
{
    // for (int i = 0; i < NumSectors; ++i)
    //     rwLock[i] = new RWLock("synRWLock");
    semaphore = new Semaphore("synch disk", 0);
    lock = new Lock("synch disk lock");
    disk = new Disk(name, DiskRequestDone, (int)this);
#ifdef CACHE
    cache = new Cache[CACHE_SIZE];
#endif
}

//----------------------------------------------------------------------
// SynchDisk::~SynchDisk
// 	De-allocate data structures needed for the synchronous disk
//	abstraction.
//----------------------------------------------------------------------

SynchDisk::~SynchDisk()
{

    // delete[] rwLock;
    delete disk;
    delete lock;
    delete semaphore;
#ifdef CACHE
    delete[] cache;
#endif
}

//----------------------------------------------------------------------
// SynchDisk::ReadSector
// 	Read the contents of a disk sector into a buffer.  Return only
//	after the data has been read.
//
//	"sectorNumber" -- the disk sector to read
//	"data" -- the buffer to hold the contents of the disk sector
//----------------------------------------------------------------------

void SynchDisk::ReadSector(int sectorNumber, char *data)
{
    lock->Acquire(); // only one disk I/O at a time
#ifdef CACHE
    int i = 0;
    for (; i < CACHE_SIZE && !(cache[i].valid && cache[i].sector == sectorNumber); ++i)
        ;                //先查找整个cache
    if (i == CACHE_SIZE) //没找到,调用页面置换算法
    {
        disk->ReadRequest(sectorNumber, data);
        semaphore->P(); // wait for interrupt
        int swap = -1;
        for (i = 0; i < CACHE_SIZE && cache[i].valid; ++i) //先找cache中valid为FALSE的置换
            ;
        if (i == CACHE_SIZE) //cache满了，置换最后一项
        {
            swap = i - 1;
            for (i = 1; i < CACHE_SIZE; ++i) //FIFO前移
                cache[i - 1] = cache[i];
        }
        else //cache没满
            swap = i;

        cache[swap].valid = TRUE;
        cache[swap].sector = sectorNumber;
        bcopy(data, cache[swap].data, SectorSize);
        // printf("cache miss, swap cache[%d] by %d\n", swap, sectorNumber);
    }
    else //找到了，直接读cache中的数据
    {
        // printf("cache[%d] hit\n", i);
        bcopy(cache[i].data, data, SectorSize);
        // disk->HandleInterrupt();
    }
#else //original verison
    disk->ReadRequest(sectorNumber, data);
    semaphore->P(); // wait for interrupt
#endif
    lock->Release();
}

//----------------------------------------------------------------------
// SynchDisk::WriteSector
// 	Write the contents of a buffer into a disk sector.  Return only
//	after the data has been written.
//
//	"sectorNumber" -- the disk sector to be written
//	"data" -- the new contents of the disk sector
//----------------------------------------------------------------------

void SynchDisk::WriteSector(int sectorNumber, char *data)
{
    lock->Acquire(); // only one disk I/O at a time
#ifdef CACHE
    for (int i = 0; i < CACHE_SIZE; ++i)
    {
        if (cache[i].sector == sectorNumber)
        {
            // printf("cache[%d] is deleted\n", i);
            cache[i].valid = FALSE;
            break;
        }
    }
#endif
    disk->WriteRequest(sectorNumber, data);
    semaphore->P(); // wait for interrupt
    lock->Release();
}

//----------------------------------------------------------------------
// SynchDisk::RequestDone
// 	Disk interrupt handler.  Wake up any thread waiting for the disk
//	request to finish.
//----------------------------------------------------------------------

void SynchDisk::RequestDone()
{
    semaphore->V();
}

#ifdef CACHE
Cache::Cache()
{
    valid = FALSE;
    sector = 0;
}
#endif
