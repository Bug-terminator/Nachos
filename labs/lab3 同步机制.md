# lab3 同步机制

李糖 2001210320

## 任务完成情况

| Exercises  | Y/N  |
| ---------- | ---- |
| Exercise   | Y    |
| Exercise   | Y    |
| Exercise   | Y    |
| Exercise   | Y    |
| *challenge | Y    |

## Exercise1 调研

> 调研Linux中实现的同步机制。

- [Locking in the Linux Kernel](https://www.kernel.org/doc/htmldocs/kernel-locking/locks.html)

在`include/linux`路径下：

互斥锁

- [`mutex.h`](https://github.com/torvalds/linux/blob/master/include/linux/mutex.h)

其他锁

- [`spinlock.h`](https://github.com/torvalds/linux/blob/33def8498fdde180023444b08e12b72a9efed41d/include/linux/spinlock.h)
- [`rwlock.h`](https://github.com/torvalds/linux/blob/6f0d349d922ba44e4348a17a78ea51b7135965b1/include/linux/rwlock.h)
- ...

### 结论

Linux在内核中实现了很多种类不同的锁，通常情况下用于系统软件和硬件的管理。而对于用户级进程，据我所知一般是使用[ptherads库](https://github.com/GerHobbelt/pthread-win32)。

## Exercise2 源代码阅读

> 阅读下列源代码，理解Nachos现有的同步机制。
>
> code/threads/synch.h和code/threads/synch.cc
>
> code/threads/synchlist.h和code/threads/synchlist.cc

### code/threads/synch.h

实现了信号量机制。

| 成员变量/函数 | 描述                                                         |
| ------------- | ------------------------------------------------------------ |
| int value     | 信号量值，永远大于等于0                                      |
| List *queue   | 在P()中被阻塞的线程队列，等待value>0之后被唤醒               |
| void P()      | 当value == 0时，将currentThread放入queue中。挂起currentThread并执行其他线程；当value>0时，value-- |
| void V()      | 判断queue中是否有元素，如果有，则唤醒，并将其加入就绪队列；value++ |

## Exercise3 实现锁和条件变量

> 可以使用sleep和wakeup两个原语操作（注意屏蔽系统中断），也可以使用Semaphore作为唯一同步原语（不必自己编写开关中断的代码）。

在开头关中断，在结尾开中断，保证整个程序是原子操作。

### Pthreads库

> pthreads提供了两种同步机制：mutex和condition

- POSIX Threads Programming

  - [Mutex Variables](https://computing.llnl.gov/tutorials/pthreads/#Mutexes)

  - [Condition Variables](https://computing.llnl.gov/tutorials/pthreads/#ConditionVariables)

