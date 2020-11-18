// progtest.cc
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"

//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

void StartProcess(char *filename)
{
    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;

    if (executable == NULL)
    {
        printf("Unable to open file %s\n", filename);
        return;
    }
    space = new AddrSpace(executable);
    currentThread->space = space;

    delete executable; // close file

    space->InitRegisters(); // set the initial register values
    space->RestoreState();  // load page table register

    machine->Run(); // jump to the user progam
    ASSERT(FALSE);  // machine->Run never returns;
                    // the address space exits
                    // by doing the syscall "exit"
}

// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void ConsoleTest(char *in, char *out)
{
    char ch;

    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);

    for (;;)
    {
        readAvail->P(); // wait for character to arrive
        ch = console->GetChar();
        console->PutChar(ch); // echo it!
        writeDone->P();       // wait for write to finish
        if (ch == 'q')
            return; // if q, quit
    }
}

//lab2 实现多线程 
//初始化地址空间

Thread *
initThreadSpace(OpenFile *executable, int number)
{
    printf("Creating user program thread %d\n", number);

    char ThreadName[20];
    sprintf(ThreadName, "User program %d", number);
    Thread *thread = new Thread(strdup(ThreadName), 1);
    AddrSpace *space;
    space = new AddrSpace(executable);
    thread->space = space;

    return thread;
}


void runUserProg(int number)
{
    printf("Running user program thread %d\n", number);
    currentThread->space->InitRegisters(); // set the initial register values
    currentThread->space->RestoreState();  // load page table register
    // currentThread->space->PrintState();    // debug usage
    machine->Run();                        // jump to the user progam
    ASSERT(FALSE);                         // machine->Run never returns;
                                           // the address space exits
                                           // by doing the syscall "exit"
}

#define N 3
void startMultiProcess(char *filename)
{
    OpenFile *executable = fileSystem->Open(filename);

    if (executable == NULL)
    {
        printf("Unable to open file %s\n", filename);
        return;
    }
    //创建一个thread指针数组
    Thread *thread[N];
    //创建三个一样的线程,并插入就绪队列
    for (int i = 0; i < N; ++i)
    {
        thread[i] = initThreadSpace(executable, i);
        thread[i]->Fork(runUserProg, i);
    }
    delete executable; // close file
    currentThread->Yield();
}