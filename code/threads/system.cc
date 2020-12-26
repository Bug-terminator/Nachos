// system.cc
//	Nachos initialization and cleanup routines.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"

//lab1 线程管理
Thread *threadPtr[maxThreadNum];
bool isAllocatable[maxThreadNum];
int threadCount = 0;
char *transEnum[threadStatusNum] = {"JUST_CREATED", "RUNNING", "READY", "BLOCKED"}; //ts方法用到
// This defines *all* of the global data structures used by Nachos.
// These are all initialized and de-allocated by this file.

Thread *currentThread;       // the thread we are running now
List *threadsToBeDestroyed_list; // the thread that just finished
Scheduler *scheduler;        // the ready list
Interrupt *interrupt;        // interrupt status
Statistics *stats;           // performance metrics
Timer *timer;                // the hardware timer device,
                             // for invoking context switches
//lab6 

//lab4
bool verbose = FALSE;
// #ifdef FILESYS_NEEDED
FileSystem *fileSystem;
// #endif

#ifdef FILESYS //暂时注释
SynchDisk *synchDisk;
#endif

// #ifdef USER_PROGRAM // requires either FILESYS or FILESYS_STUB//暂时注释 lab5
Machine *machine;   // user program memory and registers
// #endif

#ifdef NETWORK
PostOffice *postOffice;
#endif

// External definition, to allow us to take a pointer to this function
extern void Cleanup();

//----------------------------------------------------------------------
// TimerInterruptHandler
// 	Interrupt handler for the timer device.  The timer device is
//	set up to interrupt the CPU periodically (once every TimerTicks).
//	This routine is called each time there is a timer interrupt,
//	with interrupts disabled.
//
//	Note that instead of calling Yield() directly (which would
//	suspend the interrupt handler, not the interrupted thread
//	which is what we wanted to context switch), we set a flag
//	so that once the interrupt handler is done, it will appear as
//	if the interrupted thread called Yield at the point it is
//	was interrupted.
//
//	"dummy" is because every interrupt handler takes one argument,
//		whether it needs it or not.
//----------------------------------------------------------------------
// static void
// TimerInterruptHandler(int dummy)
// {
//     if (interrupt->getStatus() != IdleMode)
//         interrupt->YieldOnReturn();
// }

//in system.cc
static void
TimerInterruptHandler(int dummy)
{
    // DEBUG('c', " << random Context Switch (stats->totalTicks = %d) >>\n", stats->totalTicks);
    if (interrupt->getStatus() != IdleMode)
    {
#ifdef ROUND_ROBIN //时间片轮转

        currentThread->totalRunningTime--;
        currentThread->timeSlice--;
        if (currentThread->totalRunningTime >= 0)
            // cout << "A time interrupt occered, current time = " << stats->totalTicks << " ,thread " << currentThread->getTID() << " has runned " << 10 - currentThread->totalRunningTime << " unit time."<< endl;
            // DEBUG("t" ,)
            if (!currentThread->timeSlice)
            {
                currentThread->timeSlice = 5; //重新获取时间片，并加入就绪队列
                interrupt->YieldOnReturn();
            }
#else //原始版本
        printf("===============Random context switch, Ticks = %d===============\n",stats->totalTicks);
        scheduler->Print();
        interrupt->YieldOnReturn();
#endif
    }
}

//----------------------------------------------------------------------
// Initialize
// 	Initialize Nachos global data structures.  Interpret command
//	line arguments in order to determine flags for the initialization.
//
//	"argc" is the number of command line arguments (including the name
//		of the command) -- ex: "nachos -d +" -> argc = 3
//	"argv" is an array of strings, one for each command line argument
//		ex: "nachos -d +" -> argv = {"nachos", "-d", "+"}
//----------------------------------------------------------------------
void Initialize(int argc, char **argv)
{

    //lab1 初始化
    for (int i = 0; i < maxThreadNum; ++i)
    {
        isAllocatable[i] = true;
        threadPtr[i] = NULL;
    }

    
    int argCount;
    char *debugArgs = "";
    bool randomYield = FALSE;

// #ifdef USER_PROGRAM//暂时注释
    bool debugUserProg = FALSE; // single step user program
// #endif
#ifdef FILESYS_NEEDED
    bool format = FALSE; // format disk
#endif
#ifdef NETWORK
    double rely = 1; // network reliability
    int netname = 0; // UNIX socket name
#endif

    for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount)
    {
        argCount = 1;
        if (!strcmp(*argv, "-d"))
        {
            if (argc == 1)
                debugArgs = "+"; // turn on all debug flags
            else
            {
                debugArgs = *(argv + 1);
                argCount = 2;
            }
        }
        else if (!strcmp(*argv, "-rs"))
        {
            ASSERT(argc > 1);
            RandomInit(atoi(*(argv + 1))); // initialize pseudo-random
                                           // number generator
            randomYield = TRUE;
            argCount = 2;
        }
// #ifdef USER_PROGRAM//暂时注释
        if (!strcmp(*argv, "-s"))
            debugUserProg = TRUE;
// #endif
#ifdef FILESYS_NEEDED
        if (!strcmp(*argv, "-f"))
            format = TRUE;
#endif
#ifdef NETWORK
        if (!strcmp(*argv, "-l"))
        {
            ASSERT(argc > 1);
            rely = atof(*(argv + 1));
            argCount = 2;
        }
        else if (!strcmp(*argv, "-m"))
        {
            ASSERT(argc > 1);
            netname = atoi(*(argv + 1));
            argCount = 2;
        }
#endif
    }

    DebugInit(debugArgs);        // initialize DEBUG messages
    stats = new Statistics();    // collect statistics
    interrupt = new Interrupt;   // start up interrupt handling
    scheduler = new Scheduler(); // initialize the ready queue
    if (randomYield)             // start the timer (if needed)//lab3 不使用时间片轮转
        timer = new Timer(TimerInterruptHandler, 0, randomYield);

    threadsToBeDestroyed_list = new List();

    // We didn't explicitly allocate the current thread we are running in.
    // But if it ever tries to give up the CPU, we better have a Thread
    // object to save its state.
    currentThread = new Thread("main");
    currentThread->setStatus(RUNNING);

    interrupt->Enable();
    CallOnUserAbort(Cleanup); // if user hits ctl-C

// #ifdef USER_PROGRAM//暂时注释
    machine = new Machine(debugUserProg); // this must come first
// #endif

#ifdef FILESYS //暂时注释
    synchDisk = new SynchDisk("DISK");
#endif

#ifdef FILESYS_NEEDED //暂时注释
    fileSystem = new FileSystem(format);
#endif

#ifdef NETWORK
    postOffice = new PostOffice(netname, rely, 10);
#endif
}

//----------------------------------------------------------------------
// Cleanup
// 	Nachos is halting.  De-allocate global data structures.
//----------------------------------------------------------------------
void Cleanup()
{
    if(!verbose)
    printf("\nCleaning up...\n");
#ifdef NETWORK
    delete postOffice;
#endif

#ifdef USER_PROGRAM
    delete machine;
#endif

#ifdef FILESYS_NEEDED
    delete fileSystem;
#endif

#ifdef FILESYS
    delete synchDisk;
#endif

    delete timer;
    delete scheduler;
    delete interrupt;

    Exit(0);
}
