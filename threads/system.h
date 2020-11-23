// system.h
//	All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H
#define maxThreadNum 128
#define threadStatusNum 4

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "timer.h"
#include <iostream>
#include <vector>
using namespace std;
//新增
extern Thread *threadPtr[];
extern bool isAllocatable[];
extern int threadCount;
extern char *transEnum[];

// Initialization and cleanup routines
extern void Initialize(int argc, char **argv); // Initialization,
											   // called before anything else
extern void Cleanup();						   // Cleanup, called when
											   // Nachos is done.

extern Thread *currentThread;		// the thread holding the CPU
extern Thread *threadToBeDestroyed; // the thread that just finished
extern Scheduler *scheduler;		// the ready list
extern Interrupt *interrupt;		// interrupt status
extern Statistics *stats;			// performance metrics
extern Timer *timer;				// the hardware alarm clock

#ifdef USER_PROGRAM
#include "machine.h"
extern Machine *machine; // user program memory and registers
#endif

#ifdef FILESYS_NEEDED // FILESYS or FILESYS_STUB
#include "filesys.h"
extern FileSystem *fileSystem;
#endif

#ifdef FILESYS //为了代码补全，暂时注释 lab4
#include "synchdisk.h"
extern SynchDisk *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice *postOffice;
#endif

#endif // SYSTEM_H
