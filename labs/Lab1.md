# LAB1 线程机制和线程调度实现

## 调研Linux的进程控制块

Linux的线程管理通过task_struct结构体完成，它包含了一个进程所需的所有信息，如打开文件/进程状态/地址空间等。它定义在include/linux/sched.h头文件中。

### 进程状态

```cpp
  volatile long state;    /* -1 unrunnable, 0 runnable, >0 stopped */
```

state成员变量的取值范围如下：

```cpp
/*
  * Task state bitmask. NOTE! These bits are also
  * encoded in fs/proc/array.c: get_task_state().
  *
  * We have two separate sets of flags: task->state
  * is about runnability, while task->exit_state are
  * about the task exiting. Confusing, but this way
  * modifying one set can't modify the other one by
  * mistake.
  */
#define TASK_RUNNING 0
#define TASK_INTERRUPTIBLE 1
#define TASK_UNINTERRUPTIBLE 2
#define __TASK_STOPPED 4
#define __TASK_TRACED 8

/* in tsk->exit_state */
#define EXIT_DEAD 16
#define EXIT_ZOMBIE 32
#define EXIT_TRACE (EXIT_ZOMBIE | EXIT_DEAD)

/* in tsk->state again */
#define TASK_DEAD 64
#define TASK_WAKEKILL 128 /** wake on signals that are deadly **/
#define TASK_WAKING 256
#define TASK_PARKED 512
#define TASK_NOLOAD 1024
#define TASK_STATE_MAX 2048

/* Convenience macros for the sake of set_task_state */
#define TASK_KILLABLE (TASK_WAKEKILL | TASK_UNINTERRUPTIBLE)
#define TASK_STOPPED (TASK_WAKEKILL | __TASK_STOPPED)
#define TASK_TRACED (TASK_WAKEKILL | __TASK_TRACED)
```

### 五个互斥状态

state的取值只能是这五个状态之一，state不可能同时处于五个状态中的任意两个。

|         状态         |                             描述                             |
| :------------------: | :----------------------------------------------------------: |
|     TASK_RUNNING     |      表示进程正在执行，或者已经准备就绪，等待处理机调度      |
|  TASK_INTERRUPTIBLE  | 表示进程因等待某种资源而被挂起（阻塞态），一旦资源就绪，进程就会转化为TASK_RUNNING态 |
| TASK_UNINTERRUPTIBLE | 与TASK_INTERRUPTIBLE状态类似，同样表示进程因等待某种资源而阻塞。二者唯一的区别在于：前者可以通过一些信号或者外部中断来唤醒，而后者只能通过等待的资源就绪被唤醒。这种状态很少被用到，但是不代表它没有用，实际上这种状态很有用，特别是对于驱动刺探相关的硬件过程至关重要。 |
|     TASK_STOPPED     | 进程被停止执行，当进程接收到SIGSTOP、SIGTTIN、SIGTSTP或者SIGTTOU信号之后就会进入该状态 |
|     TASK_TRACED      | 表示进程被debugger等进程监视，进程执行被调试程序所停止，当一个进程被另外的进程所监视，每一个信号都会让进城进入该状态 |

### 两个终止状态

有两个状态既可以被state使用，也可以被exit_exit使用

```cpp
/* task state */
int exit_state;
int exit_code, exit_signal;
```

|    状态     |                             描述                             |
| :---------: | :----------------------------------------------------------: |
| EXIT_ZOMBIE | 僵尸进程是当子进程比父进程先结束，而父进程又没有回收子进程，释放子进程占用的资源，此时子进程将成为一个僵尸进程。如果父进退出 ，子进程被init接管，子进程退出后init会回收其占用的相关资源，但如果父进程不退出，子进程所占有的进程ID将不会被释放，如果系统中僵尸进程过多，会占用大量的PID，导致PID不足，这是僵尸进程的危害，应当避免 |
|  EXIT_DEAD  |                    进程正常结束的最终状态                    |

### 进程的转换过程如下图

![](C:\Users\litang\OneDrive\图片\Saved Pictures\进程状态转换.png)

### 进程标识符PID

```cpp
pid_t pid;  
pid_t tgid;  
```

unix通过pid来唯一地标识进程，Linux希望pid与线程相关联，同时希望同一组线程拥有相同的pid，所以Linux引入了tgid（线程组）的概念。只有领头线程的pid成员变量会被赋值，其他线程的pid成员不会被赋值，所以getpid()方法返回的是线程的tgid值，这点值得注意。

### 进程内核栈

```cpp
void *stack;  
```

#### 内核栈与线程描述符

对每个进程，Linux内核都把两个不同的数据结构紧凑的存放在一个单独为进程分配的内存区域中

- 一个是内核态的进程堆栈，
- 另一个是紧挨着进程描述符的小数据结构thread_info，叫做线程描述符。

Linux把thread_info（线程描述符）和内核态的线程堆栈存放在一起，这块区域通常是8192K（占两个页框），其实地址必须是8192的整数倍。

在linux/arch/x86/include/asm/page_32_types.h中

```cpp
#define THREAD_SIZE_ORDER    1
#define THREAD_SIZE        (PAGE_SIZE << THREAD_SIZE_ORDER)
```

> 需要注意的是，**内核态堆栈**仅用于内核例程，Linux内核另外为中断提供了单独的**硬中断栈**和**软中断栈**

下图中显示了在物理内存中存放两种数据结构的方式。线程描述符驻留与这个内存区的开始，而栈顶末端向下增长。 下图摘自ULK3,进程内核栈与进程描述符的关系如下图：

![](C:\Users\litang\OneDrive\图片\Saved Pictures\20160512131035840.png)

在这个图中，esp寄存器是CPU栈指针，用来存放栈顶单元的地址。在80x86系统中，栈起始于顶端，并朝着这个内存区开始的方向增长。从用户态刚切换到内核态以后，进程的内核栈总是空的。因此，esp寄存器指向这个栈的顶端。一旦数据写入堆栈，esp的值就递减。

### 内核栈数据结构描述thread_info和thread_union

Linux内核中使用一个联合体来表示一个进程的线程描述符和内核栈：

```cpp
union thread_union
{
    struct thread_info thread_info;
    unsigned long stack[THREAD_SIZE/sizeof(long)];
};
```

### 进程标记

```cpp
unsigned int flags; /* per process flags, defined below */  
```

反应进程状态的信息，但不是运行状态，用于内核识别进程当前的状态，以备下一步操作

flags成员的可能取值如下，这些宏以PF(ProcessFlag)开头

```cpp
 /*
* Per process flags
*/
#define PF_EXITING      0x00000004      /* getting shut down */
#define PF_EXITPIDONE   0x00000008      /* pi exit done on shut down */
#define PF_VCPU         0x00000010      /* I'm a virtual CPU */
#define PF_WQ_WORKER    0x00000020      /* I'm a workqueue worker */
#define PF_FORKNOEXEC   0x00000040      /* forked but didn't exec */
#define PF_MCE_PROCESS  0x00000080      /* process policy on mce errors */
#define PF_SUPERPRIV    0x00000100      /* used super-user privileges */
#define PF_DUMPCORE     0x00000200      /* dumped core */
#define PF_SIGNALED     0x00000400      /* killed by a signal */
#define PF_MEMALLOC     0x00000800      /* Allocating memory */
#define PF_NPROC_EXCEEDED 0x00001000    /* set_user noticed that RLIMIT_NPROC was exceeded */
#define PF_USED_MATH    0x00002000      /* if unset the fpu must be initialized before use */
#define PF_USED_ASYNC   0x00004000      /* used async_schedule*(), used by module init */
#define PF_NOFREEZE     0x00008000      /* this thread should not be frozen */
#define PF_FROZEN       0x00010000      /* frozen for system suspend */
#define PF_FSTRANS      0x00020000      /* inside a filesystem transaction */
#define PF_KSWAPD       0x00040000      /* I am kswapd */
#define PF_MEMALLOC_NOIO 0x00080000     /* Allocating memory without IO involved */
#define PF_LESS_THROTTLE 0x00100000     /* Throttle me less: I clean memory */
#define PF_KTHREAD      0x00200000      /* I am a kernel thread */
#define PF_RANDOMIZE    0x00400000      /* randomize virtual address space */
#define PF_SWAPWRITE    0x00800000      /* Allowed to write to swap */
#define PF_NO_SETAFFINITY 0x04000000    /* Userland is not allowed to meddle with cpus_allowed */
#define PF_MCE_EARLY    0x08000000      /* Early kill for mce process policy */
#define PF_MUTEX_TESTER 0x20000000      /* Thread belongs to the rt mutex tester */
#define PF_FREEZER_SKIP 0x40000000      /* Freezer should not count it as freezable */
#define PF_SUSPEND_TASK 0x80000000      /* this thread called freeze_processes and should not be frozen */
```

### 表示进程亲属关系的成员

```cpp
/*
 * pointers to (original) parent process, youngest child, younger sibling,
 * older sibling, respectively.  (p->father can be replaced with
 * p->real_parent->pid)
 */
struct task_struct __rcu *real_parent; /* real parent process */
struct task_struct __rcu *parent; /* recipient of SIGCHLD, wait4() reports */
/*
 * children/sibling forms the list of my natural children
 */
struct list_head children;      /* list of my children */
struct list_head sibling;       /* linkage in my parent's children list */
struct task_struct *group_leader;       /* threadgroup leader */
```

|     字段     |                             描述                             |
| :----------: | :----------------------------------------------------------: |
| real_parent  | 指向其父进程，如果创建它的父进程不再存在，则指向PID为1的init进程 |
|    parent    | 指向其父进程，当它终止时，必须向它的父进程发送信号。它的值通常与real_parent相同 |
|   children   |        表示链表的头部，链表中的所有元素都是它的子进程        |
|   sibling    |                用于把当前进程插入到兄弟链表中                |
| group_leader |                  指向其所在进程组的领头进程                  |

### 进程调度

#### 优先级

```cpp
int prio, static_prio, normal_prio;
unsigned int rt_priority;
```

|    字段     |                        描述                        |
| :---------: | :------------------------------------------------: |
| static_prio | 用于保存静态优先级，可以通过nice系统调用来进行修改 |
| rt_priority |                 用于保存实时优先级                 |
| normal_prio |            值取决于静态优先级和调度策略            |
|    prio     |                 用于保存动态优先级                 |

实时优先级范围是0到MAX_RT_PRIO-1（即99），而普通进程的静态优先级范围是从MAX_RT_PRIO到MAX_PRIO-1（即100到139）。值越大静态优先级越低。

```cpp
/*  http://lxr.free-electrons.com/source/include/linux/sched/prio.h#L21  */
#define MAX_USER_RT_PRIO    100
#define MAX_RT_PRIO     MAX_USER_RT_PRIO

/* http://lxr.free-electrons.com/source/include/linux/sched/prio.h#L24  */
#define MAX_PRIO        (MAX_RT_PRIO + 40)
#define DEFAULT_PRIO        (MAX_RT_PRIO + 20)
```

#### 调度策略

```cpp
/*
* Scheduling policies
*/
#define SCHED_NORMAL            0
#define SCHED_FIFO              1
#define SCHED_RR                2
#define SCHED_BATCH             3
/* SCHED_ISO: reserved but not implemented yet */
#define SCHED_IDLE              5
#define SCHED_DEADLINE          6
```

|                |                                                              |              |
| :------------: | :----------------------------------------------------------: | :----------: |
|      字段      |                             描述                             | 所在调度器类 |
|  SCHED_NORMAL  | （也叫SCHED_OTHER）用于普通进程，通过CFS调度器实现。SCHED_BATCH用于非交互的处理器消耗型进程。SCHED_IDLE是在系统负载很低时使用 |     CFS      |
|  SCHED_BATCH   | SCHED_NORMAL普通进程策略的分化版本。采用分时策略，根据动态优先级(可用nice()API设置），分配 CPU 运算资源。注意：这类进程比上述两类实时进程优先级低，换言之，在有实时进程存在时，实时进程优先调度。但针对吞吐量优化 |     CFS      |
|   SCHED_IDLE   | 优先级最低，在系统空闲时才跑这类进程(如利用闲散计算机资源跑地外文明搜索，蛋白质结构分析等任务，是此调度策略的适用者） |     CFS      |
|   SCHED_FIFO   | 先入先出调度算法（实时调度策略），相同优先级的任务先到先服务，高优先级的任务可以抢占低优先级的任务 |      RT      |
|    SCHED_RR    | 轮流调度算法（实时调度策略），后者提供 Roound-Robin 语义，采用时间片，相同优先级的任务当用完时间片会被放到队列尾部，以保证公平性，同样，高优先级的任务可以抢占低优先级的任务。不同要求的实时任务可以根据需要用sched_setscheduler()API 设置策略 |      RT      |
| SCHED_DEADLINE | 新支持的实时进程调度策略，针对突发型计算，且对延迟和完成时间高度敏感的任务适用。基于Earliest Deadline First (EDF) 调度算法 |              |

#### 进程地址空间

```cpp
/*  http://lxr.free-electrons.com/source/include/linux/sched.h?V=4.5#L1453 */
struct mm_struct *mm, *active_mm;
/* per-thread vma caching */
u32 vmacache_seqnum;
struct vm_area_struct *vmacache[VMACACHE_SIZE];
#if defined(SPLIT_RSS_COUNTING)
struct task_rss_stat    rss_stat;
#endif

/*  http://lxr.free-electrons.com/source/include/linux/sched.h?V=4.5#L1484  */
#ifdef CONFIG_COMPAT_BRK
unsigned brk_randomized:1;
#endif
```

| 字段           | 描述                                                         |
| -------------- | ------------------------------------------------------------ |
| mm             | 进程所拥有的用户空间内存描述符，内核线程无的mm为NULL         |
| active_mm      | active_mm指向进程运行时所使用的内存描述符， 对于普通进程而言，这两个指针变量的值相同。但是内核线程kernel thread是没有进程地址空间的，所以内核线程的tsk->mm域是空（NULL）。但是内核必须知道用户空间包含了什么，因此它的active_mm成员被初始化为前一个运行进程的active_mm值。 |
| brk_randomized | 用来确定对随机堆内存的探测。参见[LKML](http://lkml.indiana.edu/hypermail/linux/kernel/1104.1/00196.html)上的介绍 |
| rss_stat       | 用来记录缓冲信息                                             |

> **因此如果当前内核线程被调度之前运行的也是另外一个内核线程时候，那么其mm和avtive_mm都是NULL**

### Linux与Nachos的异同

#### Linux中的PCB包含以下的内容

1. PID
2. 特征信息
3. 进程状态
4. 优先级
5. 通信状态
6. 现场保护区
7. 资源需求、分配控制信息
8. 进程实体信息
9. 其他（工作单位、工作区、文件信息）

#### Nachos中的PCB实现

PCB的实现`thread.h`中，其中仅包含以下信息：

1. stackTop and stack, 表示当前进程所占的栈顶和栈底
2. machineState， 保留未在CPU上运行的进程的寄存器状态
3. status， 表示当前进程的状态

#### 异同

Nachos是一个羽量级的Linux，很多功能待完善。

## Exercise1 源代码阅读

### main.cc

**从注释可以看出，main()的作用是引导操作系统内核/检查命令行参数/初始化数据结构/调用测试流程（可选）。**

|                方法                 |                             描述                             |
| :---------------------------------: | :----------------------------------------------------------: |
| DEBUG(char flag, char *format, ...) | 如果启用了该标志（flag），则打印一条调试消息。像printf一样，只是在前面多了一个额外的参数 |
|  Initialize(int argc, char **argv)  | 初始化Nachos全局数据结构。解释命令行参数，以确定用于初始化的标志。“ argc”是命令行参数的数量（包括名称）；“ argv”是一个字符串数组，每个命令行参数占argv中的一个位置 |
|            ThreadTest()             |            调用某个测试流程，可以在case中自定义。            |

### ThreadTest.cc

|        变量/方法        |                             描述                             |
| :---------------------: | :----------------------------------------------------------: |
|         testnum         | 测试编号，可以在命令行中输入`./nachos -q testnum`，以执行对应的测试流程 |
| SimpleThread(int which) | 循环5次，每次迭代都会让出CPU给另外一个就绪进程，which是用来标识线程的简单标志，可用于调试。 |
|      ThreadTest1()      | 通过派生一个线程调用SimpleThread，然后自己调用SimpleThread，在两个线程交替执行 |
|      ThreadTest()       |                根据testnum调用对应的测试流程                 |

### thread.h/thread.cc

|        宏        |                             描述                             |
| :--------------: | :----------------------------------------------------------: |
| MachineStateSize | 上下文切换时需要保存的CPU寄存器状态，SPARC和MIPS仅需要10个，而Snake需要18个，为了能够在每个系统上执行，取值18。 |
|    StackSize     | 线程执行的私有栈空间大小，大小为4096个字(in words, not Bytes）。 |
|   ThreadStatus   | 枚举了线程的状态：JUST_CREATED(创建）,  RUNNING(执行),  READY(就绪），BLOCKED(阻塞)。 |
| STACK_FENCEPOST  |   将其放在执行堆栈的顶部，以检测堆栈溢出，值为`0xdeadbeef`   |

|                      成员变量/方法                      |                             描述                             |
| :-----------------------------------------------------: | :----------------------------------------------------------: |
|                        stackTop                         |                      当前线程的栈顶指针                      |
|                      machineState                       |        栈顶指针所需要的所有寄存器，全部保存在该数组中        |
|             Thread::Thread(char *debugName)             | 初始化线程控制块，以便我们可以调用Thread::Fork。"threadName"是任意字符串，可用于调试。 |
|                    Thread::~Thread()                    | 取消分配线程。 注意：当前线程**不能**直接删除自身， 由于它仍在堆栈中运行，因此我们还需要删除它的堆栈。注意：如果这是主线程，则由于未分配堆栈，因此无法删除堆栈-在启动Nachos时会自动获取堆栈。 |
|      Thread::Fork(VoidFunctionPtr func, void *arg)      | 调用(* func)(arg)，允许调用方和被调用方并发执行。**注意**：尽管定义只允许将单个整数参数传递给过程，但是可以通过将多个参数设为结构的字段，并将指针作为“ arg”传递给该结构来传递多个参数。可以按以下步骤实现：1.分配堆栈2.初始化堆栈，以便对SWITCH的调用将使其运行3.将线程放在就绪队列中“ func”是要同时运行的过程。“ arg”是要传递给该过程的单个参数。 |
|                 Thread::CheckOverflow()                 | 检查线程的堆栈，以查看其是否超出了为其分配的空间。注意：Nachos不会捕获所有堆栈溢出条件。 换句话说，程序可能仍会由于溢出而崩溃。如果得到奇怪的结果（例如没有代码的段错误），那么可能**需要**增加堆栈大小。可以通过不在堆栈上放置大型数据结构来避免堆栈溢出。(不要这样做：void foo（）{int bigArray [10000]; ...} |
|                    Thread::Finish ()                    | 线程完成分叉过程时，由ThreadRoot调用Thread::Finish。**注意**：系统不会立即取消分配线程数据结构或执行堆栈，因为线程仍在堆栈中运行！相反，系统设置“ threadToBeDestroyed”，以便一旦系统在其他线程的上下文中运行，Scheduler::Run()将调用析构函数。 **注意**：该操作为原子操作，禁止中断。 |
|                    Thread::Yield ()                     | 如果有其他就绪线程，让出CPU，再将该线程添加到就绪列表的末尾，以便最终对其进行重新调度。 **注意**：此操作为原子操作，禁止中断。 |
|                    Thread::Sleep ()                     | 放弃CPU，因为当前线程在等待同步变量（信号量，锁定或条件）时被阻塞。最终，某个线程将唤醒该线程，并将其放回到就绪队列中，以便可以对其进行重新调度。 |
| Thread::StackAllocate (VoidFunctionPtr func, void *arg) | 分配并初始化执行堆栈。堆栈使用ThreadRoot的初始堆栈框架初始化，该框架如下： 启用中断 通话(* fun)(carg)调用Thread::Finish |

## Exercise2 扩展线程的数据结构

为了增加用户ID和线程ID，在thread.h中新增`uID`和`tID`两个变量，在`system.h/system.cc`中新增一个大小为128的数组`isAllocatable[128]`,并在`initialize()`方法中对其初始化为全0，**确保**每次nachos执行时该数组会被初始化。最后增加一个变量`threadCount`用于记录线程数，初始化为0。

### 维护tID

在`Thread::Thread()`方法中遍历`isAllocatable`数组，将第一个遇到的未分配的线程ID分配给当前线程，同时threadCount增加1。

在`Thread::~Thread()`方法中将`isAllocatable[this->tID]`重新置为0，然后令`threadCount`减少1。

### 维护uID

与同学讨论之后我们认为nachos没有维护用户ID的条件，因此我将uID设为一个默认值0。如果要维护uID，应该增加在登录时输入用户名和密码的功能，并建立用户名和uID的映射关系。

## Exercise3 增加全局线程管理机制

### 使nachos中最多存在128个线程

在调用`thread::thread()`方法之前先检查`threadCount`的值是否小于128，如果是，代表还能够继续分配线程；如果不是，代表已经达到线程数量的最大值，屏幕打印错误信息。

### 仿照Linux中PS命令，增加一个功能TS(Threads Status)，能够显示当前系统中所有线程的信息和状态

#### 题意

题目的要求是在terminal中输入`./nacos -TS`能够打印出线程的信息，为了实现这一功能，应该在main.cc中增加一个"-TS"的判断。如果用户输入了`./nacos -TS`那么调用TS()方法。

#### 定义Thread* threadPtr[128]

在`system.h/system.cc`中新增一个大小为128的类型为Thread*的数组threadPtr，在`initialize()`中初始化为NULL。其作用是建立线程指针和tID之间的映射关系(map)。维护方法和isAllocatable类似，在`Thread::Thread()`方法中，每分配一个tID就让`threadPtr[tID] = this`。在`Thread::~Thread()`方法中令`threadPtr[tID] = NULL`。

#### 定义TS()方法

在`ThreadTest.cc`中定义`TS()`方法，遍历`isAllocatable`数组，每当遇到非零的元素时，通过`threadPtr`找到其对应的线程指针，屏幕打印其对应的`uID/tID/name/status`。

## Exercise4 源代码阅读

### scheduler.h/scheduler.cc

|                成员方法                |                             描述                             |
| :------------------------------------: | :----------------------------------------------------------: |
|         Scheduler::Scheduler()         |                        初始化就绪队列                        |
|        Scheduler::~Scheduler()         |                         释放就绪队列                         |
| Scheduler::ReadyToRun (Thread *thread) |                    将线程加入就绪队列队尾                    |
|      Scheduler::FindNextToRun ()       |          返回就绪队列队首线程，若队列为空，返回NULL          |
|  Scheduler::Run (Thread *nextThread)   | 将CPU分配给nextThread。保存旧线程的状态，并加载新线程的状态，通过调用机器依赖上下文切换例程SWITCH。注意：threadToBeDestroyed是正在等待释放的线程，因为它们不能自己释放自己，所以必须通过除自己之外的线程来释放。 |
|           Scheduler::Print()           |                    打印就绪队列，用于调试                    |

### timer.h/timer.cc

|        成员变量/方法         |                             描述                             |
| :--------------------------: | :----------------------------------------------------------: |
|          randomize           |                  设置是否需要使用随机时间片                  |
|           handler            |                      计时器中断处理程序                      |
|             arg              |                   中断处理函数所需要的参数                   |
|         Timer::Timer         | 初始化硬件计时器设备。保存要在每个中断上调用的位置，然后安排计时器开始生成中断。 |
|    Timer::TimerExpired()     | 用于模拟硬件计时器设备生成的中断。安排下一个中断，并调用中断处理程序。 |
| Timer::TimeOfNextInterrupt() | 返回下一次硬件计时器将引起中断的时间。 如果启用了随机化，则将其设置为（伪）随机延迟。 |



## *Chalenge 实现时间片轮转算法

通过阅读stats.h发现可以通过totalTicks来控制线程的执行时间，也可以输出当前的系统时间，单位为ms，此外时间片间隔默认为100ms， 可以修改`TimerTicks`宏来自定义时间片。

| 位置                                      | 新增变量/语句                                                | 描述                                                         |
| ----------------------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| system.h/system.cc::TimerInterruptHandler | TimerInterruptHandler(int dummy){cout << currTime is << stats->totalTicks << endl;currentThread->Yeild();} | 时间中断处理函数，每当时间中断发生时，currentThread->Yeild(). |
|                                           |                                                              |                                                              |
|                                           |                                                              |                                                              |
|                                           |                                                              |                                                              |

### 成果演示

我修改了simpleThread方法的内容：当系统时间小于3000ms时无限循环。在ThreadTest方法中new了十个线程，从main函数开始，逐个调用simpleThread方法，预期结果：每隔100ms会发生一次线程切换。

```cpp
//time-slicing algorithm,in threadTest.cc
void mySimlpe2(int dummy)
{
    cout << "current time is : " << stats->totalTicks << ", thread " << currentThread->getTID() << " is running." << endl;
    while(stats->totalTicks < 3000);
}
void timeSlicingTest()
{
    for(int i = 0;i<10;++i)
    {
        Thread* t = new Thread("forked");
        t->Fork(mySimlpe2,(void*)1);
    }
    mySimlpe2(1);
}
//in system.cc
static void
TimerInterruptHandler(int dummy)
{
    if (interrupt->getStatus() != IdleMode)
        interrupt->YieldOnReturn();
    cout << "Current time is " << stats->totalTicks << endl;
    currentThread->Yield();
}

//in scheduler.cc把readytorun方法还原一下，别用优先级抢占
void
Scheduler::ReadyToRun (Thread *thread)
{
    DEBUG('t', "Putting thread %s on ready list.\n", thread->getName());

    thread->setStatus(READY);
    readyList->Append((void *)thread);
}

```

