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

//lab4 synchConcole
void SynchConsoleTest(char *in, char *out)
{
    char ch;
    SynchConsole *synchConsole = new SynchConsole(in, out);
    for (;;)
    {
        ch = synchConsole->GetChar();
        synchConsole->PutChar(ch);
        if (ch == 'q')
            return; // if q, quit
    }
}

//lab4 pipe
SynchConsole *sc_writer, *sc_reader;
char ch;
void Pipe_Writer()
{
    fileSystem->Create("pipe", 0); //创建管道
    sc_writer = new SynchConsole(NULL, "pipe");
    do
    {
        ch = sc_writer->GetChar();
        sc_writer->PutChar(ch);

    } while (ch != 'q'); //向管道中输出数据
}

void Pipe_Reader()
{
    sc_reader = new SynchConsole("pipe", NULL);
    do
    {
        ch = sc_reader->GetChar();
        sc_reader->PutChar(ch);
    } while (ch != 'q'); //从管道中获取数据
}

void PipeTest()
{
    Pipe_Writer();
    Pipe_Reader();
}