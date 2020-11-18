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

//lab3 实现lock
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

//lab3 环境变量
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
    DEBUG('c', "%s has blocked %s.", getName(), currentThread->getName()); //c means condition
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
    ASSERT(conditionLock->isHeldByCurrentThread())
    //唤醒进程
    if (!queue->IsEmpty())
    {
        Thread *thread = (Thread *)queue->Remove();
        scheduler->ReadyToRun(thread);
        DEBUG('c', "%s wakes up %s.", getName(), thread->getName());
    }
    interrupt->SetLevel(oldLevel);
}

void Condition::Broadcast(Lock *conditionLock)
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    //环境变量的所有者必须为当前线程
    ASSERT(conditionLock->isHeldByCurrentThread())
    DEBUG('c', "%s broadcast :\n ", getName());
    //唤醒所有进程
    while (!queue->IsEmpty())
    {
        Thread *thread = (Thread *)queue->Remove();
        scheduler->ReadyToRun(thread);
        DEBUG('c', "%s wakes up.\t", thread->getName());
    }
    interrupt->SetLevel(oldLevel);
}

//lab3 challenge barrier
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
    DEBUG('b', "%s reached %s with remain = %d.\n", currentThread->getName(), name, remain); //b means barrier
    if (!remain)
    {
        DEBUG('b', "All threads reached %s.\n", name);
        condition->Broadcast(mutex);
        //重置barrier
        remain = threadNum;
    }
    else
        condition->Wait(mutex);
    mutex->Release();
    interrupt->SetLevel(oldLevel);
}
