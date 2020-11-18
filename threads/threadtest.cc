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
//git test, I add somen sentence here; result shows it worked.
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

#ifdef LAB1
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
#endif

//----------------------------------------------------------------------
// Lab3 Exercise4 生产者消费者问题
// 在main中new一个生产者和一个消费者
// 消费者：每次从buffer中取一个元素
// 生产者：每次生产一个元素放入buffer
// buff满,生产者阻塞，buff空，消费者阻塞
// 保证生产者和消费者互斥访问buffer
//----------------------------------------------------------------------
#define BUFFER_SIZE 5                  //buffer的大小
#define THREADNUM_P (Random() % 4 + 1) //生产者数,不超过4
#define THREADNUM_C (Random() % 4 + 1) //消费者数,不超过4
#define TESTTIME 1500                  //本次测试的总时间

vector<char> buffer;     //方便起见，用STL作为buffer
Lock *mutex;             //mutex->缓冲区的互斥访问
Condition *full, *empty; //full->生产者的条件变量，empty->消费者的条件变量

//消费者线程
void comsumer(int dummy)
{
    while (stats->totalTicks < TESTTIME) //约等于while(true),这样写可以在有限的时间内结束
    {
        //保证对缓冲区的互斥访问
        mutex->Acquire();
        //缓冲区孔，阻塞当前消费者
        if (!buffer.size())
        {
            printf("Buffer is empty with size %d.\n", buffer.size());
            empty->Wait(mutex);
        }

        //消费一个缓冲区物品
        ASSERT(buffer.size());
        buffer.pop_back();
        printf("Thread \"%s\" gets an item.\n", currentThread->getName());

        //若存在阻塞的生产者，将他们中的一个释放
        if (buffer.size() == BUFFER_SIZE - 1)
            full->Signal(mutex);

        //释放锁
        mutex->Release();
        interrupt->OneTick(); //系统时间自增
    }
}

//生产者线程
void producer(int dummy)
{
    while (stats->totalTicks < TESTTIME) //约等于while(true),这样写可以在有限的时间内结束
    {
        //保证对缓冲区的互斥访问
        mutex->Acquire();

        //缓冲区满，阻塞当前线程
        if (buffer.size() == BUFFER_SIZE)
        {
            printf("Buffer is full with size %d.\n", buffer.size());
            full->Wait(mutex);
        }

        //生产一个物品放入缓冲区
        ASSERT(buffer.size() < BUFFER_SIZE);
        buffer.push_back('0');
        printf("Thread \"%s\" puts an item.\n", currentThread->getName());

        //若存在阻塞的消费者，将他们中的一个释放
        if (buffer.size() == 1)
            empty->Signal(mutex);

        //释放锁
        mutex->Release();
        interrupt->OneTick(); //系统时间自增
    }
}
void Lab3ProducerAndComsumer()
{
    printf("Random created %d comsumers, %d producers.\n", THREADNUM_C, THREADNUM_P);

    full = new Condition("Full_condition");   //初始化full
    empty = new Condition("Empty_condition"); //初始化empty
    mutex = new Lock("buffer_mutex");         //初始化mutex

    Thread *threadComsumer[THREADNUM_C];
    Thread *threadProducer[THREADNUM_P];

    //初始化消费者
    for (int i = 0; i < THREADNUM_C; ++i)
    {
        char threadName[20];
        sprintf(threadName, "Comsumer %d", i); //给线程命名
        threadComsumer[i] = new Thread(strdup(threadName));
        threadComsumer[i]->Fork(comsumer, 0);
    }
    //初始化生产者
    for (int i = 0; i < THREADNUM_P; ++i)
    {
        char threadName[20];
        sprintf(threadName, "Producer %d", i); //给线程命名
        threadProducer[i] = new Thread(strdup(threadName));
        threadProducer[i]->Fork(producer, 0);
    }
    while (!scheduler->isEmpty())
        currentThread->Yield(); //跳过main的执行

    //结束
    printf("Producer consumer test Finished.\n");
}

//----------------------------------------------------------------------
// lab3 Challenge1 Barrier
// new 4 个线程，每个线程分别对4个全局变量进行赋值
// 共分三个阶段,每个阶段赋值不同，但是在相同的阶段中，
// 每个线程对对应的数组元素赋值是相同的
//----------------------------------------------------------------------

#define THREADNUM 4 //线程数
#define PHASENUM 3  //测试的阶段数
int num[THREADNUM];
Barrier *barrier;

//为每个变量赋值，变量与线程一一对应
void assignValue(int i) //i代表数组线标
{
    //每个循环代表一个阶段
    for (int j = 1; j <= PHASENUM; ++j)
    {
        num[i] = j;
        printf("Phase %d: thread \"%s\" finished assignment, num[%d] = %d.\n", j, currentThread->getName(), i, j);
        barrier->stopAndWait(); //线程暂时被barrier阻塞，并等待所有线程抵达
    }
}

void Lab3Barrier()
{
    barrier = new Barrier("barrier", THREADNUM);
    Thread *threads[THREADNUM];
    //初始化线程和数组,并加入就绪队列
    for (int i = 0; i < THREADNUM; ++i)
    {
        num[i] = 0;
        char threadName[30];
        sprintf(threadName, "Barrier test %d", i); //给线程命名
        threads[i] = new Thread(strdup(threadName));
        threads[i]->Fork(assignValue, i);
    }
    while (!scheduler->isEmpty())
        currentThread->Yield(); //跳过main的执行
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
#ifdef LAB1
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
#endif
    case 5:
        Lab3Barrier();
        break;
    case 6:
        Lab3ProducerAndComsumer();
        break;
    default:
        printf("No test specified.\n");
        break;
    }
}
