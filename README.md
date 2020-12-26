## Overview

本学期的*Nachos*之旅到此就告一段落了，回想这一路，痛是真的：我经历了一开始装环境的折磨，还有内存管理和文件系统两个*lab*的地狱级难度，毫不夸张地说，这学期我基本把所有的时间都给了*Nachos*。但是我的成长也是真的。作为一个跨考的学生，一开始，我连源代码都读不懂（如果让一个没学过*MakeFile*语法的人去理解*Nachos*是如何编译的，并让这个没有*c*语言基础的人一上手就看一堆的*#ifdef*代码，那么他一定会崩溃）。此外，因为我没有学过汇编，而汇编恰恰就*Nachos*精华的精华，如果能理解那几个.s文件（*start.s*/switch.s)，那么对*Nachos*的理解将更深一步。而现在，我基本可以照着手册看一些简单的汇编代码了（我认为这个进步是巨大的，这意味着我理解了计算机是如何工作的）。关于*Debug*，系统级的*bug*是最难解决的，在*Nachos*中一般会产生*Segmentation* *Fault*。简简单单两个单词，让我数次夜不能寐。就是因为它简单，没有一句多余的废话，让我根本不知道错在哪里。后来，慢慢地，我居然也会用*gdb*查看堆栈信息了，确实有点儿神奇，因为一开始，我确实只会用*printf*打印调试信息。

如果把*Nachos*比作一个项目的话，那么上手这个项目，我花了近一个月。同时我也参考了大量前人做出的实验结果，其中对我帮助最大的还是*github*上的李大为学长，他不仅在完成了大部分实验内容的前提下，还能自定义*nachos* *Debug*的颜色（这点在突显了他的嵌入式基础），最重要的是通过各种*Debug*符号巧妙控制调试信息（一开始我真的没懂*Debug*函数是干嘛用的，这点反映出他对源代码的理解），是他能够把自己做的*Nachos*开源出来，才得以让我能够站在巨人的肩膀上更进一步，我在此表示诚挚的谢意。[*DaviddwLee84/Nachos*](https://github.com/daviddwlee84/OperatingSystem/blob/master/Lab/Lab4_VirtualMemory/README.md)

同时我也决定将我做的实验报告全部开源，让学弟学妹们在做*Nachos*的时候少走弯路。

## Nachos Labs

| Lab   | Subject                                                      | Detail                                                     |
| :---- | :----------------------------------------------------------- | :--------------------------------------------------------- |
| Lab 1 | [Thread Mechanism&Scheduling](labs\Lab1.md) | Multi-thread management and scheduling                     |
| Lab 2 | [Synchronization Mechanism](labs\Lab3.md)           | Concurrency, mutex lock and semaphore                      |
| Lab 3 | [Virtual Memory](labs\Lab2.md)                      | TLB, demand paging, replacement algorithm and user program |
| Lab 4 | [File System](labs\Lab4.md)                         | Improve current file system                                |
| Lab 5 | [System Call](labs\Lab5.md)                         | Implement all system calls                                 |

| *Lab*                                      | 主要内容                          |
| ------------------------------------------ | --------------------------------- |
| [*Lab1* 线程调度](labs/lab1.md)            | 线程机制和线程调度实现            |
| [*Lab2* 虚拟内存]()                        | *TLB*，请求分页调度算法和用户程序 |
| [*Lab3* 同步机制]()                        | 信号量，锁，条件变量，读写锁      |
| [*Lab4* 文件系统]()                        | 多级索引，多级目录，管道，*Cache* |
| [*Lab5* 系统调用](labs/FileSystem/lab5.md) | 实现系统调用                      |
| [*Lab6* *Shell*实现]()                     | 实现自己的Shell                   |
| [Lab7 通信机制]()                          | 实现通信机制                      |

