// threadtest.cc
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield,
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void SimpleThread(int which)
{
    int num;

    for (num = 0; num < 5; num++)
    {
        printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread", 1);

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

//lab1自定义函数

void mySimple(int n)
{
    for (int i = 0; i < n; ++i)
    {
        cout << currentThread->getName() << " thread " << currentThread->getTID() << " is running." << endl;
        currentThread->Yield();
    }
}
void TS()
{
    cout << "print all threads info :" << endl;
    cout << "name          tID   uID   status" << endl;
    for (int i = 0; i < maxThreadNum; ++i)
    {
        if (!isAllocatable[i])
        {
            Thread *p = threadPtr[i];
            cout << p->getName() << "          " << p->getTID() << "  " << p->getuID() << "  " << transEnum[p->getStatus()] << endl;
        }
    }
}
void myThreadTest()
{
    DEBUG('t', "Entering myThreadTest");
    for (int i = 0; i < 130; ++i)
    {
        if (threadCount < maxThreadNum)
        {
            Thread *t = new Thread("forked thread", 1);
            t->Fork(mySimple, 2);
        }
        else
            cout << "threadNum exceed 128, thread create has been terminated." << endl;
    }
    mySimple(2);
    TS();
}

//lab1 exercise5
void mySimple1(int n)
{
    cout << "thread " << currentThread->getTID() << " is running." << endl;
    if (currentThread && currentThread->getPrio() > 0)
    {
        Thread *t = new Thread("forked thread", currentThread->getPrio() - 1);
        t->Fork(mySimple1, 1);
    }
}

void myThreadTest1()
{
    DEBUG('t', "Entering myThreadTest1");
    // for (int i = 8; i < 11; ++i)
    // {
    //     if (threadCount < maxThreadNum)
    //     {
    //         Thread *t = new Thread("forked thread", i);
    //         t->Fork(mySimple1, 1);
    //     }
    //     else
    //         cout << "threadNum exceed 128, thread create has been terminated." << endl;
    // }
    mySimple1(1);
}

//time-slicing algorithm,in threadTest.cc
void mySimlpe2(int dummy)
{
    while (currentThread->totalRunningTime > 0)
    {
        interrupt->OneTick();

        // cout << "curr time is " << stats->totalTicks++ << endl;
    }
}
void timeSlicingTest()
{
        Thread *t = new Thread("forked", 1);
        t->Fork(mySimlpe2, 1);
    mySimlpe2(1);
}
//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void ThreadTest()
{
    switch (testnum)
    {
    case 1:
        ThreadTest1();
        break;
    case 2:
        myThreadTest();
        break;
    case 3:
        myThreadTest1();
        break;
    case 4:
        timeSlicingTest();
        break;
    default:
        printf("No test specified.\n");
        break;
    }
}
