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
//git test, I add somen sentence here
#include "copyright.h"
#include "system.h"
#include "synch.h"

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



/* lab3 challenge 实现barrier
new 4 个线程，分别对4个全局变量
进行赋值，然后再main中计算它们的
和，共分三个阶段，每个阶段计算一次和 
测试使用random seed随机时间片*/
#define THREADNUM 4
int num[THREADNUM];
Barrier *barrier;
//为每个变量赋值，变量与线程一一对应
void assignValue(int i)
{
    
    //第一阶段
    num[i] = 1;
    printf("First stage : %s finished assignment, num[%d] = %d.\n", currentThread->getName(), i, i);
    // currentThread->Yield(); //启用random seed需要注释此句, -rs
    barrier->stopAndWait();
    //第二阶段；
    num[i] = 2;
    printf("Second stage : %s finished assignment, num[%d] = %d.\n", currentThread->getName(), i, i);
    // currentThread->Yield();
    barrier->stopAndWait();
    //第三阶段
    num[i] = 3;
    printf("Third stage : %s finished assignment, num[%d] = %d.\n", currentThread->getName(),i ,i);
    // currentThread->Yield();
    barrier->stopAndWait();
}

void Lab3Barrier()
{
    barrier = new Barrier("barrier", THREADNUM+1);//main也需要算进去
    Thread *threads[THREADNUM];
    //初始化线程和数组,并加入就绪队列
    for (int i = 0; i < THREADNUM; ++i)
    {
        num[i] = 0;
        char threadName[30];
        sprintf(threadName,"Barrier test %d", i);//给线程命名
        threads[i] = new Thread(strdup(threadName));
        threads[i]->Fork(assignValue, i);
    }
    //初始信息,以下代码可以用一个for循环改写，但是这个框架更易于改写成各个阶段不同的计算（todo），所以保留
    printf("Main is running.\n");
    printf("Default stage : sum = %d\n", num[0]+num[1]+num[2]+num[3]);
    // currentThread->Yield();
    barrier->stopAndWait();
    
    //第一阶段
    printf("Main is running.\n");
    printf("First stage : sum = %d\n", num[0]+num[1]+num[2]+num[3]);
    // currentThread->Yield();
    barrier->stopAndWait();
    
    //第二阶段
    printf("Main is running.\n");
    printf("Second stage : sum = %d\n", num[0]+num[1]+num[2]+num[3]);
    // currentThread->Yield();
    barrier->stopAndWait();

    //第三阶段
    printf("Main is running.\n");
    printf("Third stage : sum = %d\n", num[0]+num[1]+num[2]+num[3]);
    // currentThread->Yield();
    barrier->stopAndWait();

    //结束
    printf("Barrier test Finished.\n");

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
    case 5:
        Lab3Barrier();
        break;
    default:
        printf("No test specified.\n");
        break;
    }
}
