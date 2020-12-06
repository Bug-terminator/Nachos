// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "machine.h"
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions
//	are in machine.h.
//----------------------------------------------------------------------

// Machine *machine;
// TranslationEntry *pageTable, *tlb;

//lab2 缺页异常和Lazy-loading
//朴素页面置换算法
int NaiveReplacement(int vpn)
{
    int ppn = -1;
    for (int i = 0; i < machine->pageTableSize; i++)
    {
        if (i == vpn)
            continue; //跳过自己
        if (machine->pageTable[i].valid)
        {
            if (!machine->pageTable[i].dirty)
            {
                ppn = machine->pageTable[i].physicalPage;
                break;
            }
        }
    }
    if (ppn == -1)
    {
        for (int i = 0; i < machine->pageTableSize; i++)
        {
            if (i == vpn)
                continue;
            if (machine->pageTable[i].valid)
            {
                machine->pageTable[i].valid = FALSE;
                ppn = machine->pageTable[i].physicalPage;

                //写回硬盘
                OpenFile *vm = fileSystem->Open("VirtualMemory");
                vm->WriteAt(&(machine->mainMemory[ppn * PageSize]), PageSize, i * PageSize);
                delete vm; // close file
                break;
            }
        }
    }
    return ppn;
}

TranslationEntry
PageMisstHandler(int vpn)
{
    int ppn = machine->bitmap->allocateMem();
    if (ppn == -1)
    {
        ppn = NaiveReplacement(vpn);
    }
    machine->pageTable[vpn].physicalPage = ppn;

    //从磁盘中加载页
    DEBUG('a', "Demand paging: loading page from virtual memory!\n");
    OpenFile *vm = fileSystem->Open("VirtualMemory"); // This file is created in userprog/addrspace.cc
    vm->ReadAt(&(machine->mainMemory[ppn * PageSize]), PageSize, vpn * PageSize);
    delete vm; // close the file

    //设置页面参数
    machine->pageTable[vpn].valid = TRUE;
    machine->pageTable[vpn].use = FALSE;
    machine->pageTable[vpn].dirty = FALSE;
    machine->pageTable[vpn].readOnly = FALSE;
}

//lab2 系统调用不会让pc前进，所以我们需要手动让pc自增
void advancePC(void)
{
    machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
    machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
    machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
}

//lab2 Print TLB Status
void Machine ::PrintTLBStatus(void)
{
#ifdef USE_TLB
#ifdef TLB_FIFO
    DEBUG('r', "FIFO : ");
#endif
#ifdef TLB_LRU
    DEBUG('r', "LRU : ");
#endif
    // Lab2: Used for calculate TLB Hit rate (debug usage)
    DEBUG('r', "TLB Hit: %d, Total Visit: %d, TLB Hit Rate: %.2lf%%\n",
          tlbHitCnt, tlbVisitCnt, (double)(100 * 1.0 * tlbHitCnt / tlbVisitCnt));
#endif
}

void TLBMissHandler(int virtAddr)
{
    unsigned int vpn = (unsigned)virtAddr / PageSize;
    unsigned int tlbExchangeIndex = -1;
    for (int i = 0; i < TLBSize; ++i)
    {
        if (!machine->tlb[i].valid)
        {
            tlbExchangeIndex = i;
            break;
        }
    }
    //tlb满
    if (tlbExchangeIndex == -1)
    {
        DEBUG('a', "tlb get max size.\n");

#ifdef TLB_FIFO
        tlbExchangeIndex = TLBSize - 1;
        for (int i = 0; i < TLBSize - 1; ++i)
        {
            machine->tlb[i] = machine->tlb[i + 1];
        }
#endif

#ifdef TLB_LRU
        unsigned int min = __INT_MAX__;
        for (int i = 0; i < TLBSize; ++i)
        {
            if (machine->tlb[i].lastVisitedTime < min)
            {
                min = machine->tlb[i].lastVisitedTime;
                tlbExchangeIndex = i;
            }
        }
#endif
    }
    DEBUG('a', "tlb[%d] has been exchanged by PageTable[%d].\n", tlbExchangeIndex, vpn);
    if (machine->pageTable[vpn].valid)
        machine->tlb[tlbExchangeIndex] = machine->pageTable[vpn]; //将页表中的页面加载到tlb中，因为报错了，所以pc还没增加，所以Nachos会重新查找虚拟地址，这次在tlb中就会命中了
    else
        PageMisstHandler(vpn);
    // #ifdef TLB_LRU
    // machine->tlb[tlbExchangeIndex].lastVisitedTime = stats->totalTicks;
    // #endif
    //注意，此时只是加载页面，还没访问页面，所以不能更新访问时间。
}

// ==================Lab5===================
// ==================Lab5===================
// ==================Lab5===================
// ==================Lab5===================
// ==================Lab5===================




//----------------------------------------------------------------------
// FileSystemHandler
// 	Handling file system related system call.
//----------------------------------------------------------------------
#ifndef FileNameMaxLength
#define FileNameMaxLength 20 // originally define in filesys/directory.h，changed in lab4
#endif

// Helper function to get file name using ReadMem for Create and Open syscall
char *getNameFromNachosMemory(int address)
{
    int position = 0;
    int data;
    char *name = new char[FileNameMaxLength + 1]; //使用char[] int stack while new char[] in heap.
    do
    {
        // each time read one byte
        machine->ReadMem(address + position, 1, &data);
        name[position++] = (char)data;
    } while (data != 0);
    return name;
}

void FileSystemHandler(int type)
{
    if (type == SC_Create)
    { // void Create(char *name)
        char *name = getNameFromNachosMemory(machine->ReadRegister(4));
        fileSystem->Create(name, 0);
        cout << "Success create file " << name << endl;
        delete[] name; //回收内存
    }
    else if (type == SC_Open)
    {
        char *name = getNameFromNachosMemory(machine->ReadRegister(4)); // OpenFileId Open(char *name);
        machine->WriteRegister(2, (OpenFileId)fileSystem->Open(name));  // return result
        cout << "Success open file " << name << endl;
        delete[] name; //回收内存
    }
    else if (type == SC_Close)
    {                                                                   // void Close(OpenFileId id);
        OpenFile *openFile = (OpenFile *)machine->ReadRegister(4);      // OpenFile object id
        cout << "Success delete file " << (OpenFileId)openFile << endl; // release the file
        delete openFile;
    }
    else if (type == SC_Read)
    {                                                                   // int Read(char *buffer, int size, OpenFileId id);
        int addr = machine->ReadRegister(4);                            // memory starting position
        int size = machine->ReadRegister(5);                            // read "size" bytes
        OpenFile *openFile = (OpenFile *)machine->ReadRegister(6);      // OpenFile object ptr
        int numByte = openFile->Read(&machine->mainMemory[addr], size); //read from file
        machine->WriteRegister(2, numByte);                             //return value
        cout << "Success read " << numByte << "bytes from file " << (OpenFileId)openFile << endl;
    }
    else if (type == SC_Write)
    {                                                                    // void Write(char *buffer, int size, OpenFileId id);
        int addr = machine->ReadRegister(4);                             // memory starting position
        int size = machine->ReadRegister(5);                             // read "size" bytes
        OpenFile *openFile = (OpenFile *)machine->ReadRegister(6);       // OpenFile object ptr
        int numByte = openFile->Write(&machine->mainMemory[addr], size); //write to file
        cout << "Success write " << numByte << "bytes to file " << (OpenFileId)openFile << endl;
    }
}

//----------------------------------------------------------------------
//  UserProgHandler
// 	Handling user program related system call.
//----------------------------------------------------------------------
void UserProgHandler(int type)
{
    if(type == SC_Exec)
    {
        char *name = getNameFromNachosMemory(machine->ReadRegister(4));
    }
}

void ExceptionHandler(ExceptionType which)
{
    // lab6 System Call
    // The system call codes (SC_[TYPE]) is defined in userprog/syscall.h
    // The system call stubs is defined in test/start.s
    int type = machine->ReadRegister(2); // r2: the standard C calling convention on the MIPS

    if (which == SyscallException)
    {
        if (type == SC_Halt)
        {
            DEBUG('a', "Shutdown, initiated by user program.\n");
            interrupt->Halt();
        }
        else if (type == SC_Exit || type == SC_Exec || type == SC_Join || type == SC_Fork || type == SC_Yield)
        {
            UserProgHandler(type);
        }
        else if (type == SC_Create || type == SC_Open || type == SC_Write || type == SC_Read || type == SC_Close)
        {
            FileSystemHandler(type);
        }
        // Increment the Program Counter before returning.
        advancePC();
    }

    //lab2 exercise2
    else if (which = PageFaultException)
    {
        ASSERT(machine->tlb); //保证tlb正确初始化
        int badVAddr = machine->ReadRegister(BadVAddrReg);
        TLBMissHandler(badVAddr);
    }
    else
    {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}
