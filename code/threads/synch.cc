// synch.cc
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char *debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff); // disable interrupts

    while (value == 0)
    {                                         // semaphore not available
        queue->Append((void *)currentThread); // so go to sleep
        currentThread->Sleep();
    }
    value--; // semaphore available,
             // consume its value

    (void)interrupt->SetLevel(oldLevel); // re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL) // make thread ready, consuming the V immediately
        scheduler->ReadyToRun(thread);
    value++;
    (void)interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments
// Note -- without a correct implementation of Condition::Wait(),
// the test case in the network assignment won't work!

//----------------------------------------------------------------------
// Lab3 Lock
// Check my report for more information
//----------------------------------------------------------------------

Lock::Lock(char *debugName)
{
    name = debugName;
    semaphore = new Semaphore("Lock", 1);
}

Lock::~Lock()
{
    delete semaphore; //默认的析构函数无法析构new出来的对象，为了避免内存泄露，手动析构semaphore
}

bool Lock::isHeldByCurrentThread()
{
    return (currentThread == heldThread);
}

void Lock::Acquire()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    semaphore->P();
    heldThread = currentThread;
    DEBUG('l', "%s has aquired %s", heldThread->getName(), name); //l means lock
    interrupt->SetLevel(oldLevel);
}

void Lock::Release()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    ASSERT(isLocked());
    ASSERT(isHeldByCurrentThread());
    semaphore->V();
    DEBUG('l', "%s has released %s", heldThread->getName(), name);
    interrupt->SetLevel(oldLevel);
}

bool Lock::isLocked()
{
    return !semaphore->getValue();
}

//----------------------------------------------------------------------
// Lab3 Condition
// Check my report for more information
//----------------------------------------------------------------------
Condition::Condition(char *debugName)
{
    name = debugName;
    queue = new List;
}
Condition::~Condition()
{
    delete queue; //lab3 析构new出来的等待队列
}

void Condition::Wait(Lock *conditionLock)
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    //释放锁
    conditionLock->Release(); //已经断言锁已经上锁和所有者为当前线程

    //放弃cpu，并加入等待队列
    DEBUG('c', "%s has blocked thread \"%s\".\n", getName(), currentThread->getName()); //c means condition
    queue->Append(currentThread);
    currentThread->Sleep();

    //等待唤醒
    //...
    //重新获得锁
    conditionLock->Acquire();
    interrupt->SetLevel(oldLevel);
}

void Condition::Signal(Lock *conditionLock)
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    //环境变量的所有者必须为当前线程
    ASSERT(conditionLock->isHeldByCurrentThread());
    //唤醒进程
    if (!queue->IsEmpty())
    {
        Thread *thread = (Thread *)queue->Remove();
        scheduler->ReadyToRun(thread);
        DEBUG('c', "%s wakes up \"%s\".\n", getName(), thread->getName());
    }
    interrupt->SetLevel(oldLevel);
}

void Condition::Broadcast(Lock *conditionLock)
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    //环境变量的所有者必须为当前线程
    ASSERT(conditionLock->isHeldByCurrentThread());
    DEBUG('c', "broadcast : ");
    //唤醒所有进程
    while (!queue->IsEmpty())
    {
        Thread *thread = (Thread *)queue->Remove();
        scheduler->ReadyToRun(thread);
        DEBUG('c', "%s\t", thread->getName());
    }
    DEBUG('c', "\n");
    interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Lab3 Barrier
// Check my report for more information
//----------------------------------------------------------------------
Barrier::Barrier(char *debugName, int num)
{
    threadNum = num;
    name = debugName;
    remain = num;
    mutex = new Lock("Barrier mutex"); //保证对reamin的互斥访问
    condition = new Condition("Barrier condition");
}

Barrier::~Barrier()
{
    delete mutex;
    delete condition;
}

void Barrier::stopAndWait()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    mutex->Acquire(); //condition->wait(mutex)必须在锁已经锁定的情况下使用
    remain--;
    DEBUG('b', "Thread \"%s\" reached %s with remain = %d.\n", currentThread->getName(), name, remain); //b means barrier
    if (!remain)
    {
        DEBUG('b', "All threads reached %s.\n", name);
        condition->Broadcast(mutex); //释放前n-1个线程
        //重置barrier
        remain = threadNum;
        currentThread->Yield(); //最后一个线程不能用wait，否则会导致所有线程阻塞，因此，用yeild()
    }
    else
        condition->Wait(mutex); //阻塞当前线程
    mutex->Release();
    interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Lab3 RWLock
// Check my report for more information
//----------------------------------------------------------------------

RWLock::RWLock(char *debugname)
{
    DEBUG('R', "initilizing RWLock.\n");//R-RWLock
    name = debugname;
    g = new Lock("RWLock");
    COND = new Condition("Condition");
    num_readers_active = num_writers_waiting = 0;
    writer_active = false;
}

//Begin Read
void RWLock::ReaderAcquire()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    //lock g
    g->Acquire();
    //writer first
    while (num_writers_waiting > 0 || writer_active)
        COND->Wait(g);
    // increamnet number of readers
    num_readers_active++;
    //unlock g
    g->Release();
    interrupt->SetLevel(oldLevel);
}

//End Read
void RWLock::ReaderRelease()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    //lock g
    g->Acquire();
    //decreament number of readers
    num_readers_active--;
    //no readers active, notify COND
    if (!num_readers_active)
        COND->Broadcast(g);
    //unlock g
    g->Release();
    interrupt->SetLevel(oldLevel);
}

//Begin Write
void RWLock::WriterAcquire()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    //lock g
    g->Acquire();
    // increamnet number of writers
    num_writers_waiting++;
    //writer first
    while (num_readers_active > 0 || writer_active)
        COND->Wait(g);
    // decreamnet snumber of writers
    num_writers_waiting--;
    //set writer_active to true
    writer_active = true;
    //unlock g
    g->Release();
    interrupt->SetLevel(oldLevel);
}

//End Write
void RWLock::WriterRelease()
{
     IntStatus oldLevel = interrupt->SetLevel(IntOff);
    //lock g
    g->Acquire();
    //set writer_active to false
    writer_active = false;
    //notify COND
    COND->Broadcast(g);
    //unlock g
    g->Release();
    interrupt->SetLevel(oldLevel);
}
