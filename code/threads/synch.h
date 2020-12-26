// synch.h
//	Data structures for synchronizing threads.
//
//	Three kinds of synchronization are defined here: semaphores,
//	locks, and condition variables.  The implementation for
//	semaphores is given; for the latter two, only the procedure
//	interface is given -- they are to be implemented as part of
//	the first assignment.
//
//	Note that all the synchronization objects take a "name" as
//	part of the initialization.  This is solely for debugging purposes.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// synch.h -- synchronization primitives.

#ifndef SYNCH_H
#define SYNCH_H

#include "copyright.h"
#include "thread.h"
#include "list.h"

// The following class defines a "semaphore" whose value is a non-negative
// integer.  The semaphore has only two operations P() and V():
//
//	P() -- waits until value > 0, then decrement
//
//	V() -- increment, waking up a thread waiting in P() if necessary
//
// Note that the interface does *not* allow a thread to read the value of
// the semaphore directly -- even if you did read the value, the
// only thing you would know is what the value used to be.  You don't
// know what the value is now, because by the time you get the value
// into a register, a context switch might have occurred,
// and some other thread might have called P or V, so the true value might
// now be different.

class Semaphore
{
public:
  Semaphore(char *debugName, int initialValue); // set initial value
  ~Semaphore();                                 // de-allocate semaphore
  char *getName() { return name; }              // debugging assist

  void P();                        // these are the only operations on a semaphore
  void V();                        // they are both *atomic*
  int getValue() { return value; } //lab3 get the value (for Lock::isLocked())
private:
  char *name;  // useful for debugging
  int value;   // semaphore value, always >= 0
  List *queue; // threads waiting in P() for the value to be > 0
};

// The following class defines a "lock".  A lock can be BUSY or FREE.
// There are only two operations allowed on a lock:
//
//	Acquire -- wait until the lock is FREE, then set it to BUSY
//
//	Release -- set lock to be FREE, waking up a thread waiting
//		in Acquire if necessary
//
// In addition, by convention, only the thread that acquired the lock
// may release it.  As with semaphores, you can't read the lock value
// (because the value might change immediately after you read it).

class Lock
{
public:
  Lock(char *debugName);           // initialize lock to be FREE
  ~Lock();                         // deallocate lock
  char *getName() { return name; } // debugging assist

  void Acquire(); // these are the only operations on a lock
  void Release(); // they are both *atomic*

  bool isHeldByCurrentThread(); // true if the current thread
                                // holds this lock.  Useful for
                                // checking in Release, and in
                                // Condition variable ops below.
  bool isLocked();              //lab3 检查锁是否已经上锁，在condition::wait()中使用

private:
  char *name;           // for debugging
  Thread *heldThread;   //lab3 在isHeldByCurrentThread()使用
  Semaphore *semaphore; //信号量
                        // plus some other stuff you'll need to define
};

// The following class defines a "condition variable".  A condition
// variable does not have a value, but threads may be queued, waiting
// on the variable.  These are only operations on a condition variable:
//
//	Wait() -- release the lock, relinquish the CPU until signaled,
//		then re-acquire the lock
//
//	Signal() -- wake up a thread, if there are any waiting on
//		the condition
//
//	Broadcast() -- wake up all threads waiting on the condition
//
// All operations on a condition variable must be made while
// the current thread has acquired a lock.  Indeed, all accesses
// to a given condition variable must be protected by the same lock.
// In other words, mutual exclusion must be enforced among threads calling
// the condition variable operations.
//
// In Nachos, condition variables are assumed to obey *Mesa*-style
// semantics.  When a Signal or Broadcast wakes up another thread,
// it simply puts the thread on the ready list, and it is the responsibility
// of the woken thread to re-acquire the lock (this re-acquire is
// taken care of within Wait()).  By contrast, some define condition
// variables according to *Hoare*-style semantics -- where the signalling
// thread gives up control over the lock and the CPU to the woken thread,
// which runs immediately and gives back control over the lock to the
// signaller when the woken thread leaves the critical section.
//
// The consequence of using Mesa-style semantics is that some other thread
// can acquire the lock, and change data structures, before the woken
// thread gets a chance to run.

class Condition
{
public:
  Condition(char *debugName); // initialize condition to
                              // "no one waiting"
  ~Condition();               // deallocate the condition
  char *getName() { return (name); }

  void Wait(Lock *conditionLock);      // these are the 3 operations on
                                       // condition variables; releasing the
                                       // lock and going to sleep are
                                       // *atomic* in Wait()
  void Signal(Lock *conditionLock);    // conditionLock must be held by
  void Broadcast(Lock *conditionLock); // the currentThread for all of
                                       // these operations

private:
  char *name;
  List *queue;
  // plus some other stuff you'll need to define
};

// Lab3: Challenge1 Barrier

class Barrier
{
public:
  Barrier(char *debugName, int num); // 构造函数
  ~Barrier();                        // 析构函数
  char *getName() { return (name); } // 调试用

  void stopAndWait(); // 在所有线程到达之前阻塞当前线程

private:
  char *name;           // 调试用
  int remain;           // 还剩多少线程没到？
  int threadNum;        // 线程总数
  Lock *mutex;          // condition中使用的锁
  Condition *condition; // 用来阻塞线程并唤醒他们
};

//Lab3 Challenge2 读写锁
class RWLock
{
public:
  RWLock(char *debugName);           // 构造函数
  ~RWLock();                         // 析构函数
  char *getName() { return (name); } // debug辅助

  // 读者锁
  void ReaderAcquire();
  void ReaderRelease();
  // 写者锁
  void WriterAcquire();
  void WriterRelease();

private:
  char *name;              // debug用
  int num_readers_active;  //已获取锁的读者的数量
  int num_writers_waiting; //等待访问的写者数（整数）
  int writer_active;       //写者是否已获得锁（布尔值）
  Condition *COND;         //条件变量
  Lock *g;                 //互斥锁g
};

#endif // SYNCH_H
