# Lab2 虚拟内存

## 任务完成情况

| *Exercise*  | Y/N  |
| :---------: | :--: |
| *Exercise1* |  Y   |
| *Exercise2* |  Y   |
| *Exercise3* |  Y   |
| *Exercise4* |  Y   |
| *Exercise5* |  Y   |
| *Exercise6* |  Y   |
| *Exercise7* |  Y   |



## *Exercise1* 源代码阅读

### 一些值得注意的细节

system.cc中初始化了一个Machine类型的对象machine，作为Nachos的模拟硬件系统，用来执行用户的程序。Nachos机器模拟的核心部分是内存和寄存器的模拟。

nachos寄存器模拟了MIPS机(R2/3000)的全部的32个寄存器，此外还包括8个用于调试的寄存器。一些特殊寄存器描述如下表：

|   寄存器名   | 编号 |                      描述                      |
| :----------: | :--: | :--------------------------------------------: |
|   StackReg   |  29  |                  进程栈顶指针                  |
|  RetAddrReg  |  31  |                  存放返回地址                  |
|    HiReg     |  32  |               存放乘法结果高32位               |
|    LoReg     |  33  |               存放乘法结果低32位               |
|    PCReg     |  34  |               存放当前指令的地址               |
|  NextPCReg   |  35  |            存放下一条执行指令的地址            |
|  PrevPCReg   |  36  |            存放上一条执行指令的地址            |
|   LoadReg    |  37  |            存放延迟载入的寄存器编号            |
| LoadValueReg |  38  |                存放延迟载入的值                |
|  BadAddReg   |  39  | 发生Exception(异常或syscall)时用户程序逻辑地址 |

Nachos 用宿主机的一块内存模拟自己的内存。由于 Nachos 是一个教学操作系统，在内存分配上和实际的操作系统是有区别的，且作了很大幅度的简化。

Nachos中每个内存页的大小同磁盘扇区的大小相同，而整个内存的大小远远小于模拟磁盘的大小，仅为`32 * 128 = 4KB`。事实上，Nachos 的内存只有当需要执行用户程序时用户程序的一个暂存地，而作为Nachos内核的数据结构并不存放在 Nachos 的模拟内存中，而是申请 Nachos 所在宿主机的内存。所以 Nachos 的一些重要的数据结构如线程控制结构等的容量可以是无限的，不受 Nachos 模拟内存大小的限制。

> 参考资料《Nachos中文教程》

### progtest.cc

在terminal中输入`nachos -x filename`，main()将调用StartProcess(char *filename)函数，其执行流程为：打开文件->为当前线程分配地址空间（调用addrSpace函数，根据程序的数据段、程序段和栈的总和来分配地址空间，具体的流程为：将头文件内容加载到NoffHeader的结构体中，它定义了Nachos目标代码的格式，并且在必要的时候进行大小端的转换->计算用户程序所需内存空间大小，并计算所需页数->建立虚拟地址到物理地址的转换机制（目前Nachos的地址转换是完全对等的，即采用**绝对装入**的方式，只适用于单道程序）->调用machine->mainMemory初始化物理空间，将程序的数据段和代码段拷贝到内存中->关闭文件->初始化寄存器(InitRegisters)->将当前线程对应的页表始址和页目录数写入“页表寄存器”（在Machine中用两个public成员变量：`pageTable`和`pageTableSize`来实现“页表寄存器”的功能（即存储currentThread的页表始址和页表大小)->调用machine->run函数，该函数会调用OneInstruction函数逐条执行用户指令，同时调用OneTick函数，让系统时间前进，直到当前进程执行结束或者主动/被动让出CPU。

### machine.h(cc)

machine.h中定义了一组与Nachos模拟寄存器和内存有关的参数，比如物理页面大小、内存页面数、寄存器大小等等。枚举了Nachos的异常类型：

|         异常          |                      描述                       |
| :-------------------: | :---------------------------------------------: |
|      NoException      |                     无异常                      |
|   SyscallException    |  系统调用（个人意见，系统调用不应该属于异常）   |
|  PageFaultException   | 缺页异常（除了页表，使用tlb时同样会抛出此异常） |
|   ReadOnlyException   |             对只读对象做写操作异常              |
|   BusErrorException   |          地址变换结果超过物理内存异常           |
| AddressErrorException |          地址不能对齐或者地址越界异常           |
|   OverflowException   |            整型数加减法结果越界异常             |
| IllegalInstrException |                  非法指令异常                   |

|                      成员变量/函数                      |                             描述                             |
| :-----------------------------------------------------: | :----------------------------------------------------------: |
|                    char *mainMemory;                    |                  模拟内存，用于运行用户程序                  |
|              int registers[NumTotalRegs];               |                 模拟寄存器，用于运行用户程序                 |
|                 TranslationEntry *tlb;                  |                   快表，系统中只有一个快表                   |
| TranslationEntry *pageTable;unsigned int pageTableSize; | 当前线程的页表起始地址和页目录数，相当于执行“页表寄存器”的职能。 **注意**：pageTable变量在两个地方均有定义，一个是在thread.cc中，表示某个线程的页表，这个pageTable可以有多个，并且与线程是一一对应的；另一个是在machine.cc中，表示currentThread的页表，这个pageTable只有一个，且始终指向currentThread的页表。**注意**：无论是tlb还是pageTable，都不存放在模拟的mainMemory之中，而是借用宿主主机内存存放，换言之，mainMemory只用来存放用户程序指令、数据和栈。 |
|                   Machine(bool debug)                   |      初始化用户程序执行的模拟硬件环境，包括内存，寄存器      |
|                       void Run();                       |                     逐条执行用户程序指令                     |
|                  ReadRegister(int num)                  |                        读取寄存器的值                        |
|            WriteRegister(int num, int value)            |                      将value写入寄存器                       |
|        void OneInstruction(Instruction *instr);         | 执行一条用户程序指令，其步骤为：读取一条指令的二进制编码->decode()解码->获得对应的操作符opCode，rs，rt和rd寄存器->根据opCode执行相应的运算->更新寄存器PC，nextPC，prevPC的值，如果有异常，调用raiseException函数处理异常，然后调用异常处理程序处理异常。在Run函数中无限循环此函数直到用户程序执行到最后一条指令。 |
|       void DelayedLoad(int nextReg, int nextVal);       |            结束当前运行的一切东西，与mips模拟有关            |

### translate.h(cc)

定义了页目录项，实现了读写内存，地址转换的功能。

|                          变量/函数                           |                             描述                             |
| :----------------------------------------------------------: | :----------------------------------------------------------: |
|                       int virtualPage;                       |                           逻辑地址                           |
|                      int physicalPage;                       |                           物理地址                           |
|                         bool valid;                          |                       此页是否已在内存                       |
|                        bool readOnly;                        |                        此页是否为只读                        |
|                          bool use;                           |                    此页是否被读或者修改过                    |
|                         bool dirty;                          |                       此页是否被修改过                       |
|        bool ReadMem(int addr, int size, int* value);         |                           读取内存                           |
|        bool WriteMem(int addr, int size, int value);         |                           写入内存                           |
| ExceptionType Translate(int virtAddr, int* physAddr, int size,bool writing); | 实现逻辑地址到物理地址的转换，其流程为：通过virtualAddr计算出vpn和offset->通过vpn来查找ppn（在tlb或页表中查询，否则抛出异常，调用异常处理函数，目前这部分还未实现）->将ppn与offset相加，得到物理地址。 Nachos中只允许tlb和页表二选一，如果要实现二者并存，需要将ASSERT(tlb == NULL \|\| pageTable == NULL)和ASSERT(tlb != NULL \|\| pageTable != NULL)  注释掉。其实现流程与我们之前学习的地址变换机构相一致。 |

### exception.cc

定义了异常处理函数ExceptionHandler，用来对相应的异常(系统调用，地址越界，算数结果溢出等）做出处理。nachos对于异常或者系统调用的处理过程是：调用machine->raiseException函数，该函数得到异常类型和发生错误的虚拟地址，然后将发生错误的虚拟地址存入BadVAddrReg寄存器，然后调用DelayedLoad函数来结束当前运行中的所有东西，之后设置系统状态为内核态，然后调用ExceptionHandler函数进行异常处理，处理完异常之后重新将系统设置为用户态。

## *Exercise2* TLB MISS异常处理

要使用tlb，需要在`userprog/MakeFile`中添加`-DUSE_TLB`。

```
DEFINES = -DUSE_TLB
```

### Nachos是如何获取单条指令的(重要)

在 `code/machine/mipssim.cc`中

```
Machine::Run() {
    ...

    while (1) {
        OneInstruction(instr);

        ...
    }
}
```

```
Machine::OneInstruction(Instruction *instr)
{
    ...

    // Fetch instruction
    if (!machine->ReadMem(registers[PCReg], 4, &raw))
        return;			// 异常发生
    instr->value = raw;
    instr->Decode();

    ...

    // 计算下一个pc的值，但是为了避免异常发生，暂时不装载
    int pcAfter = registers[NextPCReg] + 4;

    ...


    // 现在已经成功执行指令

    ...

    // 更新PC寄存器的值，为了执行下一条指令，PC会自增4，即PC+=4
    registers[PrevPCReg] = registers[PCReg];	
    registers[PCReg] = registers[NextPCReg];
    registers[NextPCReg] = pcAfter;
}
```

如果Nachos获取指令失败（一般是因为PageFaultException），之后它并不会执行PC+4，因为`machine->ReadMem()`会返回`false`。因此，我们只需要在exceptionHandler中更新tlb，这样Nachos下一次还会执行同一条指令并且再次尝试地址转换。**这一点非常重要，我还会在Exercise7中提到它。**

### TLB miss

```cpp
bool 
Machine::ReadMem(int addr, int size, int *value)
{
	...
	exception = Translate(addr, &physicalAddress, size, FALSE);
	if (exception != NoException)
	{
		machine->RaiseException(exception, addr);
		return FALSE;
	}
	...
}
```

当执行`code/machine/translate.cc`里的 `Machine::ReadMem()` 或者 `Machine::WriteMem()`时，如果translate()返回PageFaultException，会调用`Machine::RaiseException()`并返回FALSE。

```
void 
Machine::RaiseException(ExceptionType which, int badVAddr)
{
		...
    registers[BadVAddrReg] = badVAddr;
		...
		interrupt->setStatus(SystemMode);
    ExceptionHandler(which);
    interrupt->setStatus(UserMode);
}
```

`Machine::RaiseException()`会将发生异常的虚拟地址存入`BadVAddrReg`，因此我们可以从`BadVAddrReg`中获取发生异常的虚拟地址。最后调用`exception.cc`里的`ExceptionHandler`函数对异常进行处理，目前它只有对`Halt`系统调用的处理。这里需要添加对TLB异常的`PageFaultException`处理语句:

```cpp
void ExceptionHandler(ExceptionType which)
{
    if (which = PageFaultException)
    {
				ASSERT(machine->tlb);//保证tlb存在
        int badVAddr = machine->ReadRegister(BadVAddrReg);//获取发生错误的虚拟地址
        TLBMissHandler(badVAddr);//调用页面置换算法对该虚拟地址进行处理
    }
		//syscall
    ...
}
```

## *Exercise3* 置换算法

### 准备工作

1. TLB命中率

   为了获取TLB命中率，我在`code/machine/machine.h`中定义了`tlbVisitCnt`和`tlbHitCnt`, 并在`code/machine/machine.cc`中对它们初始化为0。每当调用`code/machine/translate.cc`中的`translate`函数时`tlbVisitCnt++`，每当tlb命中时，`tlbHitCnt++`；

   ```cpp
   ExceptionType
   Machine::Translate(int virtAddr, int *physAddr, int size, bool writing)
   {
   	tlbVisitCnt++;//每当调用translate函数时，tlbVisitCnt++
   	...
   		for (entry = NULL, i = 0; i < TLBSize; i++)
   			if (tlb[i].valid && (tlb[i].virtualPage == vpn))
   			{
   				tlbHitCnt++;//每当tlb命中时，tlbHitCnt++
   				...
   			}
   		...
   }
   ```

2. 自定义用户程序

   在code/test中有几个用于测试的用户程序，其中halt太小了，只会用到三页，因此对任意的页面置换算法，使用halt测试出来的结果都是一样的；而其他几个又太大了，装载进mainMemory的时候会报错，例如matmult，但是我会在Exercise7实现了lazy-loading之后测试它。

   ```cpp
   //运行matmult报错
   vagrant@precise32:/vagrant/nachos/nachos-3.4/code/userprog$ ./nachos -d a -x ../test/matmult 
     //a 表示 address，它是Nachos自带的debug flag，后面还会用到m，表示machine
   Assertion failed: line 81, file "../userprog/addrspace.cc"
   Aborted
   ```

   所以我自己写了一个简单的用户程序——计算99乘法表`code/test/99table.c`。

   ```cpp
   /*
      Author: litang
      Description: 99table.c，用于简单测试用户程序，内容为计算99乘法表
      Since: 2020/11/8
             11:33:38
   */
   
   #include "syscall.h"
   #define N 9
   
   int main()
   {
       int table[N][N];
       for (int i = 1; i <= N; ++i)
           for (int j = 1; j <= N; ++j)
               table[i - 1][j - 1] = i * j;
       Exit(table[N - 1][N - 1]);
   }
   ```

   并且将下面的内容加入 `code/test/Makefile`

   ```makefile
   all: ... 99table
   
   ...
   
   # For Lab2: Test the TLB Hit Rate
   99table.o: 99table.c
       $(CC) $(CFLAGS) -c 99table.c
   99table: 99table.o start.o
       $(LD) $(LDFLAGS) start.o 99table.o -o 99table.coff
       ../bin/coff2noff 99table.coff 99table
   ```

   完成上述步骤之后，需要在`Nachos3.4/code/test`下重新`make`一遍。

3. 不同算法的转换

    我用了一些宏来作为不同算法切换的开关

   - TLB_FIFO
   - TLB_LRU

   > 需要注意的是：每次通过修改code/machine/Makefile中的开关来调试程序时，需要对任意代码做一些修改，然后再改回来，否则Nachos编译的时候无法检测MakeFile的改动。

### LRU算法

LRU（Least recently used，最近最少使用）。

- 用tlb来记录最近访问的页
- 若页在tlb中已存在，则将该页移到tlb头部，这样就能保证头部页是最近访问的，尾部页是最早访问的，当需要置换页的时候，直接丢弃尾部页，然后将新页插入头部即可；
- 若页在TLB中不存在，要分两种情况讨论：若TLB还有空位，需要先在页表中查找该页，且valid为True，表明此页在内存中存在，再将该页插入TLB的头部；若TLB没有空位，需要丢弃尾部页，然后将新页插入TLB头部。
- Nachos默认TLB是数组，每次插入都需要移动元素，而移动元素需要较大的时间开销。

#### 优化

- 为了减少移动元素的时间开销，可以在页表项TranslationEntry中添加成员变量lastVisitedTime，每当访问某页时，更新此成员变量为当前的totalTicks，每次需要替换页面时，需要遍历整个TLB，找出lastVisitedTime最小的表项，替换之。此方法和原始方法相比，每次更新页的访问时间时，只需要更新其lastVisitedTime即可，节约了移动元素的时间，但是增加了页表项的空间开销和查找最小的lastVisitedTime页的时间（需要遍历整个TLB，时间复杂度O(n)）。考虑到Nachos的tlb很小，只有4，因此我在本次试验中将使用此方法。
- 用数组实现LRU需要多次移动元素，而使用链表则不需要考虑这个问题。若要使用链表，则需要使用双向链表（为了维护rear指针，后面会说明），需要在TranslationEntry 中添加next和prior指针，还需要改动machine.cc中对于tlb数组的初始化，将数组改为链表。还需要一个全局变量currTLBSize来记录当前TLB的大小，维护front和rear指针指向TLB的链头和链尾，每次从尾部移除元素时，rear都需要指向前一个元素，所以需要双向链表。此方法和添加lastVisitedTime的方法相比，增加了一个额外的空间开销（next/prior指针），并增加了currTLBSize/front/rear变量，空间复杂度为O(n)+O(1）~=O(n)；另一方面，由于页面的访问时间在链表中是顺序排列的，最早访问的页就在链尾，查找最早访问页面的时间复杂度为O(1)，可以大大减少查找最早访问页的时间，故此方法是综合性能最优的。（TOBEDONE）

### FIFO算法

和LRU算法相比，FIFO算法实现比较简单。

- 维护一个FIFO队列，按照时间顺序将各数据（已分配页面）链接起来组成队列，并将置换指针指向队列的队首。
- 进行置换时，只需把置换指针所指的数据（页面）顺次换出，并把新加入的数据插到队尾即可。

### TLBMissHandler

我的TLBMissHandler实现如下：

```cpp
void TLBMissHandler(int virtAddr)
{
    unsigned int vpn = (unsigned)virtAddr / PageSize;
    unsigned int tlbExchangeIndex = -1;
    //tlb未满，直接插入空位
    for (int i = 0; i < TLBSize; ++i)
    {
        if (!machine->tlb[i].valid)
        {
            tlbExchangeIndex = i;
            break;
        }
    }
    //tlb满
    if (tlbExchangeIndex == -1)
    {
        DEBUG('a', "tlb get max size.\n");
      
#ifdef TLB_FIFO
        tlbExchangeIndex = TLBSize - 1;
      	//将后n-1个元素向前挪动一个单位，并将新元素插入队尾
        for (int i = 0; i < TLBSize - 1; ++i)
        {
            machine->tlb[i] = machine->tlb[i + 1];
        }
#endif
      
#ifdef TLB_LRU
        unsigned int min = __INT_MAX__;
      	//找lastVisitedTime最小的元素
        for (int i = 0; i < TLBSize; ++i)
        {
            if (machine->tlb[i].lastVisitedTime < min)
            {
                min = machine->tlb[i].lastVisitedTime;
                tlbExchangeIndex = i;
            }
        }
#endif
    }
    machine->tlb[tlbExchangeIndex] = machine->pageTable[vpn]; //将页表中的页面加载到tlb中
    //因为报错了，所以pc还没增加，所以Nachos会重新执行一次相同的地址转换，这次在tlb中就会命中。这点在Nachos如何获取单条指令中提到过
    // #ifdef TLB_LRU
    // machine->tlb[tlbExchangeIndex].lastVisitedTime = stats->totalTicks;
    // #endif
    //注意，此时只是加载页面，还没访问页面，所以不用更新访问时间。正确更新访问时间的位置如下：
}
```

```cpp
//更新tlb访问时间，code/machine/translate.cc
ExceptionType
Machine::Translate(int virtAddr, int *physAddr, int size, bool writing)
{
		...
		for (entry = NULL, i = 0; i < TLBSize; i++)
			if (tlb[i].valid && (tlb[i].virtualPage == vpn))
			{
				...
				tlb[i].lastVisitedTime = stats->totalTicks;//更新tlb的lastVisitedTime值
				...
				break;
			}
		...
}

```

### LRU vs. FIFO

FIFO算法的优点在于它是最简单，最公平的一种思想：**即如果一个数据是最早进入的，那么可以认为在未来访问它的概率很小。空间满的时候，最先进入的数据将被替换掉。**

缺点：判断页面置换算法优劣的指标就是缺页率。而FIFO算法的一个显著的缺点是：在某些情况下，缺页率反而会随着分配空间的增加而增加，称为Belady异常。产生Belady异常的原因是：FIFO置换算法与内存访问页面的动态特性不相容，FIFO算法置换的页面很可能是最频繁访问的页面，因此FIFO算法会导致一些页面频繁地被替换，从而导致**缺页率增加**。因此，FIFO算法逐渐被淘汰。

LRU是一种常见的缓存算法，在很多分布式系统（如Redis，Memcached）中均有广泛应用。LRU算法的思想是：**如果一个数据近期被访问过，那么可以认为在未来访问它的概率很大，空间满的时候，近期没有被访问过的数据将被替换掉**。LRU算法的优点在于它和内存访问页面的动态特性是相容的，是一种基于**时间局部性原理**的算法。因此，它的缺页率往往会低于FIFO算法。

缺点：实现比FIFO算法复杂，并且时间开销更多。

先用Nachos自带的DEBUG检查tlb在miss之后是否会进行处理，这里用Nachos自带的`code/test/halt`进行测试即可。在terminal中输入`./nachos -d a -x ../test/halt`

> debug flags : ‘a’ -- address space, ‘m’ -- machine, ‘r’--rate，测试结果如下：

```powershell
vagrant@precise32:/vagrant/nachos/nachos-3.4/code/userprog$ ./nachos -d a -x ../test/halt
Initializing address space, num pages 10, size 1280
Initializing code segment, at 0x0, size 256
Initializing stack register to 1264
Reading VA 0x0, size 4
        Translate 0x0, read: *** no valid TLB entry found for this virtual page!
tlb[0] has been exchanged by PageTable[0].//请关注这里
Reading VA 0x0, size 4
        Translate 0x0, read: phys addr = 0x0
        value read = 0c000034
Reading VA 0x4, size 4
        Translate 0x4, read: phys addr = 0x4
        value read = 00000000
Reading VA 0xd0, size 4
        Translate 0xd0, read: *** no valid TLB entry found for this virtual page!
tlb[1] has been exchanged by PageTable[1].//这里
Reading VA 0xd0, size 4
        Translate 0xd0, read: phys addr = 0xd0
        value read = 27bdffe8
Reading VA 0xd4, size 4
        Translate 0xd4, read: phys addr = 0xd4
        value read = afbf0014
Writing VA 0x4ec, size 4, value 0x8
        Translate 0x4ec, write: *** no valid TLB entry found for this virtual page!
tlb[2] has been exchanged by PageTable[9].//还有这里
Reading VA 0xd4, size 4
        Translate 0xd4, read: phys addr = 0xd4
        value read = afbf0014
Writing VA 0x4ec, size 4, value 0x8
        Translate 0x4ec, write: phys addr = 0x4ec
Reading VA 0xd8, size 4
        Translate 0xd8, read: phys addr = 0xd8
        value read = afbe0010
Writing VA 0x4e8, size 4, value 0x0
        Translate 0x4e8, write: phys addr = 0x4e8
Reading VA 0xdc, size 4
        Translate 0xdc, read: phys addr = 0xdc
        value read = 0c000030
Reading VA 0xe0, size 4
        Translate 0xe0, read: phys addr = 0xe0
        value read = 03a0f021
Reading VA 0xc0, size 4
        Translate 0xc0, read: phys addr = 0xc0
        value read = 03e00008
Reading VA 0xc4, size 4
        Translate 0xc4, read: phys addr = 0xc4
        value read = 00000000
Reading VA 0xe4, size 4
        Translate 0xe4, read: phys addr = 0xe4
        value read = 0c000004
Reading VA 0xe8, size 4
        Translate 0xe8, read: phys addr = 0xe8
        value read = 00000000
Reading VA 0x10, size 4
        Translate 0x10, read: phys addr = 0x10
        value read = 24020000
Reading VA 0x14, size 4
        Translate 0x14, read: phys addr = 0x14
        value read = 0000000c
Shutdown, initiated by user program.
Machine halting!

Ticks: total 45, idle 0, system 30, user 15
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 0
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...
```

结果显示，tlb正确调用了TLBMissHandler，并正确地从页表中加载了页面。

此外还可以发现：虽然halt初始化了10个页面，但是访问的仅有3个，分别为0/1/9号页，tlb并没有满，无法比较不同置换算法的优劣，所以我写了`99table`。这点在前面的准备工作中提到过。

分别打开FIFO和LRU的开关，即在code/userprog/MakeFile中分别加上

```CPP
DEFINES = -DTLB_LRU
DEFINES = -DTLB_FIFO
```

在terminal中输入：`./nachos -d r -x ../test/99table`

测试结果如下：

```cpp
//LRU 
vagrant@precise32:/vagrant/nachos/nachos-3.4/code/userprog$ ./nachos -d r -x ../test/99table
LRU : TLB Hit: 3423, Total Visit: 3446, TLB Hit Rate: 99.33%
Machine halting!

Ticks: total 145, idle 0, system 30, user 115
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 0
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...
```

```cpp
//FIFO
vagrant@precise32:/vagrant/nachos/nachos-3.4/code/userprog$ ./nachos -d r -x ../test/99table
FIFO : TLB Hit: 3431, Total Visit: 3479, TLB Hit Rate: 98.62%
Machine halting!

Ticks: total 145, idle 0, system 30, user 115
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 0
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...
```

#### 结论

LRU算法的命中率高于FIFO算法，换言之，LRU的缺页率略微低于FIFO算法。理论上，二者的差距应该很大，但是在实际的测试结果中显示，二者的差距只有百分之一，这是因为99table空间局部性很强，经常访问的页面就是这么几个，所以二者的差距拉不开。我们需要更大的程序来论证我们的观点，比如matmult（完成Exercise 7之后进行）。

## *Exercise4* 内存全局管理数据结构

### 位图bitMap

所谓位图，就是利用bit来记录内存的分配情况,每一位只有0和1两种状态，0表示未分配，1表示已分配。一个int型变量占4B = 32bit的物理空间，一个bool型变量占1B =8bit的物理空间，若开辟一个bool型数组来记录内存分配情况，其空间开销将为位图的八倍，故使用位图可以大大减少空间开销。

为了实现位图，我将bitMap封装成一个类，成员变量和函数如下表：

|             成员变量/函数              |                             描述                             |
| :------------------------------------: | :----------------------------------------------------------: |
|          unsigned int bitmap;          | 由于Nachos物理空间为32页，因此使用一个无符号int型变量正好可以记录内存分配情况。 |
|           int findZeroBit()            | 找到第一个为0的位，返回其下标，若没有，则返回-1；该函数使用了一些简单的位运算实现逐位检查并check该位是否为1的功能。 |
| void setBit(int index, bool zeroOrOne) | 根据bool型参数zeroOrOne的值（true表示1，false表示0），将下标为index的位设为0或1。用于内存分配/回收。 |
|             void freeMem()             |                           回收内存                           |
|           int allocateMem()            |                           分配内存                           |
|           void printBitMap()           |                  按二进制格式打印bitMap信息                  |

我在`code/machine/machine.h`中新增了一个bitMap类：

```cpp
// lab2 定义bitMap
class bitMap
{
	public:
	int findZeroBit();
	void setBit(int index, bool zeroOrOne);
	int allocateMem();
	void freeMem();
	void printBitMap();
	private:
	unsigned int bitmap;
};
```

在`code/machine/machine.cc`中实现了所有的成员函数：

```cpp
//lab2 实现bitMap
int bitMap ::findZeroBit() // 找到为0的位
{
    int index = 0;
    for (; index < NumPhysPages; index++)
    {
        if (!(bitmap >> index & 0x1))
        {
            return index;
        }
    }
    return -1;
}

void bitMap ::setBit(int index, bool zeroOrOne) //将index置0/1
{
    if (zeroOrOne)
        bitmap |= (0x1 << index); //置0
    else
        bitmap &= (0x1 << index); //置1
}

int bitMap ::allocateMem()
{
    int ppn = findZeroBit();
    if (ppn != -1)
    {
        DEBUG('p', "Using bitmap, physical page frame: %d is allocated.\n", ppn); //p代表physical page
        setBit(ppn, true);
    }
    else
        DEBUG('p', "Exceeded number of physical pages!\n");
    return ppn;
}


void bitMap::freeMem()
{
    for (int i = 0; i < machine->pageTableSize; i++)
    {
        if (machine->pageTable[i].valid)
        { 
            // Free currentThread's all page frames
            int pageFrameNum = machine->pageTable[i].physicalPage;
            setBit(pageFrameNum, false);
            DEBUG('p', "Physical page frame: %d freed.\n", pageFrameNum);
        }
    }
    DEBUG('p', "After freed, bitmap : ");
    machine->bitmap->printBitMap();
}

void bitMap::printBitMap() //c语言没有输出2进制的接口，所以用一个循环逐位输出bitmap
{
  	printf("bitmap : ");
    unsigned int shift = 1 << 31;
    for (; shift >= 0; shift >>= 1)
    {
        if (shift & bitmap)
            printf("1");
        else
            printf("0");
    }
    printf("\n");
}
```

在`code/machine/machine.h`的`machine`类中定义一个`bitmap`，并在`code/machine/machine.cc`中初始化它

```cpp
class Machine
{
		public:
    ...
    //lab2 bitmap
    bitMap *bitmap;
    ...
}
Machine::Machine(bool debug)
{
    //lab2 bitmap
    bitmap = new bitMap;
  	...
}
```

至此，我们已经有了自己的bitmap对象，并可以用它来分配内存了。

### 内存分配

在addrSpace的构造函数中：利用bitMap成员函数allocateMem来分配内存。

```cpp
AddrSpace::AddrSpace(OpenFile *executable)
{
    ...
    //初始化页表
    for (i = 0; i < numPages; i++)
    {
        pageTable[i].virtualPage = i; 
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE; 
        int ppn = machine->bitmap->allocateMem();
        if (ppn != -1)
        {
            pageTable[i].physicalPage = ppn;
            pageTable[i].valid = TRUE;
        }
        else
            pageTable[i].valid = FALSE;
    }
  	
  	DEBUG('p',"After allocate, ");//内存分配完毕，打印bitmap
    machine->bitmap->printBitMap();
  	...
}
```

### 内存回收

正如code/machine/machine.cc中的注释所言，machine->run()永远不会return，要回收addrSpace要借助系统调用Exit（在exercise5中我会详细解释Exit系统调用）。

在Exit系统调用中添加如下语句：

```cpp
//lab2 bitmap內存回收
void Exit(int exitCode)
{
#ifdef USER_PROGRAM
        machine->bitmap->freeMem();
#endif
    exit(exitCode);
}
```

### 测试

本次测试使用debug flag p，代表physical page management。在terminal中输入`./nachos -d p -x ../test/halt`，结果如下

```shell
vagrant@precise32:/vagrant/nachos/nachos-3.4/code/userprog$ ./nachos -d p -x ../test/halt
Physical page frame: 0 is allocated.
Physical page frame: 1 is allocated.
Physical page frame: 2 is allocated.
Physical page frame: 3 is allocated.
Physical page frame: 4 is allocated.
Physical page frame: 5 is allocated.
Physical page frame: 6 is allocated.
Physical page frame: 7 is allocated.
Physical page frame: 8 is allocated.
Physical page frame: 9 is allocated.
After allocated : bitmap : 00000000000000000000001111111111
Machine halting!

Ticks: total 45, idle 0, system 30, user 15
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 0
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...
Physical page frame: 0 is freed.
Physical page frame: 1 is freed.
Physical page frame: 2 is freed.
Physical page frame: 3 is freed.
Physical page frame: 4 is freed.
Physical page frame: 5 is freed.
Physical page frame: 6 is freed.
Physical page frame: 7 is freed.
Physical page frame: 8 is freed.
Physical page frame: 9 is freed.
After freed, bitmap : 00000000000000000000000000000000
```

结果显示：Nachos结束之后才释放bitmap，这显然是不对的。你知道原因吗？因为halt没有显式地调用Exit，这个Exit是在code/thread/system.cc中的cleanup()调用的，在这之前已经执行了Halt系统调用，所以屏幕打印halt信息早于bitmap回收信息。

我们应该在code/userprog/exception.cc中的exceptionHandler中添加对Halt系统调用的处理语句：

```cpp
void ExceptionHandler(ExceptionType which)
{
    ...
    if (which == SyscallException)
    {
        if (type == SC_Halt)
        {
            machine->bitmap->freeMem();
        }
    }
    ...
}
```

再次执行`./nachos -d p -x ../test/halt`，结果正确。

```cpp
vagrant@precise32:/vagrant/nachos/nachos-3.4/code/userprog$ ./nachos -d p -x ../test/halt
Physical page frame: 0 is allocated.
Physical page frame: 1 is allocated.
Physical page frame: 2 is allocated.
Physical page frame: 3 is allocated.
Physical page frame: 4 is allocated.
Physical page frame: 5 is allocated.
Physical page frame: 6 is allocated.
Physical page frame: 7 is allocated.
Physical page frame: 8 is allocated.
Physical page frame: 9 is allocated.
After allocated， bitmap : 00000000000000000000001111111111
Physical page frame: 0 is freed.
Physical page frame: 1 is freed.
Physical page frame: 2 is freed.
Physical page frame: 3 is freed.
Physical page frame: 4 is freed.
Physical page frame: 5 is freed.
Physical page frame: 6 is freed.
Physical page frame: 7 is freed.
Physical page frame: 8 is freed.
Physical page frame: 9 is freed.
After freed, bitmap : 00000000000000000000000000000000
Machine halting!

Ticks: total 45, idle 0, system 30, user 15
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 0
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...

```

### 结论

成功利用bitMap实现对内存的分配与回收。

> 其实Nachos在code/userprog/bitmap.h(cc)中已经实现了BitMap，如果以后有扩展物理内存的需求，以至于当前的unsigned int类型无法满足需求时，应该切换为Nachos自带的bitmap。

## *Exercise5*

### 线程切换和用户程序切换的关系

#### 寄存器和内存

thread.h中定义了userRegisters用于保存和恢复Nachos的模拟寄存器，定义了两个函数来保存和恢复寄存器，分别为SaveUserState函数和RestoreUserState函数。在schedule->run函数中，在Switch之前调用currentThread->SaveUserState将machine的寄存器数组拷贝到线程的userRegisters数组中，并调用currentThread->space->SaveState()函数来保存地址空间，目前函数体为空，我们需要**改写**：将TLB的内容清空：

```cpp
void AddrSpace::SaveState()
{
#ifdef USE_TLB // Lab2: Clean up TLB
    DEBUG('T', "Clean up TLB due to Context Switch!\n");
    for (int i = 0; i < TLBSize; i++)
    {
        machine->tlb[i].valid = FALSE;
    }
#endif
}
```

在Switch之后调用currentThread->RestoreUserState函数来将线程中userRegisters数组中保存的数据拷贝到machine的寄存器数组中，再调用currentThread->space->RestoreState()函数来将线程的页表**始址**拷贝到machine中的pageTable变量中。

#### 构造多线程

我在 `code/userprog/progtest.cc`中加入了startMultiProgress()函数，并注释掉了原来的startProgress(),调用方法还是输入-x。

在startMultiProgress中，会构造三个线程，并执行相同的程序。

```cpp
//lab2 多线程
#define N 3   //线程数
void startMultiProcess(char *filename)
{
    OpenFile *executable = fileSystem->Open(filename);

    if (executable == NULL)
    {
        printf("Unable to open file %s\n", filename);
        return;
    }
    //创建一个thread指针数组
    Thread *thread[N];
    //创建三个一样的线程,并插入就绪队列
    for (int i = 0; i < N; ++i)
    {
        thread[i] = initThreadSpace(executable, i);
        thread[i]->Fork(runUserProg, i);
    }
    delete executable; // close file
    currentThread->Yield();
}
```

创建线程时，需要初始化地址空间：

```cpp
//lab2 实现多线程 初始化地址空间

Thread *
initThreadSpace(OpenFile *executable, int number)
{
    printf("Creating user program thread %d\n", number);

    char *ThreadName;
    sprintf(ThreadName, "User program %d", number);
    Thread *thread = new Thread(ThreadName, 1);
    AddrSpace *space;
    space = new AddrSpace(executable);
    thread->space = space;

    return thread;
}
```

对每个用户程序，我们都需要初始化它们的寄存器和页表。

```cpp
//lab2 运行用户程序

void runUserProg(int number)
{
    printf("Running user program thread %d\n", number);
    currentThread->space->InitRegisters(); // set the initial register values
    currentThread->space->RestoreState();  // load page table register
    // currentThread->space->PrintState();    // debug usage
    machine->Run();                        // jump to the user progam
    ASSERT(FALSE);                         // machine->Run never returns;
                                           // the address space exits
                                           // by doing the syscall "exit"
}
```

### 测试

在前面的实验中我们已经知道Nachos自带的halt会占用10页物理内存，3个halt线程应该占30页。在terminal中输入`./nachos -d rtp -x ../test/halt`,

>  r表示rate of tlb hit(命中率），t是Nachos自带的thread debug flag, p表示physical page management。测试结果如下：

```cpp
vagrant@precise32:/vagrant/nachos/nachos-3.4/code/userprog$ ./nachos -d rtp -x ../test/halt
Creating user program thread 0
Physical page frame: 0 is allocated.
Physical page frame: 1 is allocated.
Physical page frame: 2 is allocated.
Physical page frame: 3 is allocated.
Physical page frame: 4 is allocated.
Physical page frame: 5 is allocated.
Physical page frame: 6 is allocated.
Physical page frame: 7 is allocated.
Physical page frame: 8 is allocated.
Physical page frame: 9 is allocated.
After allocated, bitmap : 00000000000000000000001111111111
Forking thread "User program 0" with func = 0x804d14b, arg = 0
Putting thread User program 0 on ready list.
Creating user program thread 1
Physical page frame: 10 is allocated.
Physical page frame: 11 is allocated.
Physical page frame: 12 is allocated.
Physical page frame: 13 is allocated.
Physical page frame: 14 is allocated.
Physical page frame: 15 is allocated.
Physical page frame: 16 is allocated.
Physical page frame: 17 is allocated.
Physical page frame: 18 is allocated.
Physical page frame: 19 is allocated.
After allocated, bitmap : 00000000000011111111111111111111
Forking thread "User program 1" with func = 0x804d14b, arg = 1
Putting thread User program 1 on ready list.
Creating user program thread 2
Physical page frame: 20 is allocated.
Physical page frame: 21 is allocated.
Physical page frame: 22 is allocated.
Physical page frame: 23 is allocated.
Physical page frame: 24 is allocated.
Physical page frame: 25 is allocated.
Physical page frame: 26 is allocated.
Physical page frame: 27 is allocated.
Physical page frame: 28 is allocated.
Physical page frame: 29 is allocated.
After allocated, bitmap : 00111111111111111111111111111111
Forking thread "User program 2" with func = 0x804d14b, arg = 2
Putting thread User program 2 on ready list.
Yielding thread "main"
Putting thread main on ready list.
Switching from thread "main" to thread "User program 0"
Running user program thread 0
Yielding thread "User program 0"
Putting thread User program 0 on ready list.
Switching from thread "User program 0" to thread "User program 1"
...//线程切换
Deleting thread "main"
LRU : TLB Hit: 29, Total Visit: 40, TLB Hit Rate: 72.50%
Physical page frame: 0 is freed.
Physical page frame: 1 is freed.
Physical page frame: 2 is freed.
Physical page frame: 3 is freed.
Physical page frame: 4 is freed.
Physical page frame: 5 is freed.
Physical page frame: 6 is freed.
Physical page frame: 7 is freed.
Physical page frame: 8 is freed.
Physical page frame: 9 is freed.
After freed, bitmap : 00000000000000000000000000000000
Machine halting!

Ticks: total 165, idle 0, system 130, user 35
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 0
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...
```

### 结论	

成功实现多线程。

## *Exercise6* 缺页中断

缺页时需要从硬盘读取页，Nachos没有对disk进行模拟，所以我用文件代替。

#### 虚拟存储

我用一个文件virtualMemory来装载用户程序的数据段和代码段。并对addrSpace()作了如下修改：

- pageTable[i] = FALSE;表示我们一开始并不把任何页装入内存
- 从executable中复制数据段和代码段到virtualMemory文件中

### 缺页中断处理函数

在`code/userprog/exception.cc`中，每当替换页面valid位为FALSE时，将产生缺页异常。

```cpp
void
TLBMissHandler(int virtAddr)
{
    unsigned int vpn = virtAddr / PageSize;

    // 寻找置换页
    TranslationEntry page = machine->pageTable[vpn];
    if (!page.valid) { 
        page = PageMissHandler(vpn);
    }

    ...
```

`PageMissHandler`将做如下的工作：

- 通过bitmap成员函数allocateMem()来分配mainMemory中的空闲页
- 从硬盘中加载对应页面到mainMemory中
  - 如果mainMemory满了，将调用页面置换算法，这里使用FIFO算法（我将在Exercise 7 中详细说明）

```cpp
TranslationEntry
PageMissHandler(int vpn)
{
		
    int ppn = machine->allocateFrame(); // ppn
    if (ppn == -1) { // Need page replacement
        ppn = naiveReplacement(vpn);
    }
    machine->pageTable[vpn].physicalPage = ppn;

		//从硬盘中加载页面到mainMemory
    DEBUG('a', "Demand paging: loading page from virtual memory!\n");
    OpenFile *vm = fileSystem->Open("VirtualMemory"); 
    vm->ReadAt(&(machine->mainMemory[pageFrame*PageSize]), PageSize, vpn*PageSize);
    delete vm; 

    //设置页面参数
    machine->pageTable[vpn].valid = TRUE;
    machine->pageTable[vpn].use = FALSE;
    machine->pageTable[vpn].dirty = FALSE;
    machine->pageTable[vpn].readOnly = FALSE;
}
```

## *Exercise7* *Lazy*-*loading*

### 朴素页面置换算法Naive page replacement

- 寻找mainMemory中一个dirty位为FALSE的页面进行置换
- 如果不存在，则将一个dirty位为TRUE的页面写入硬盘

```cpp
//lab2 Lazy-loading
//朴素页面置换算法
int NaiveReplacement(int vpn)
{
    int ppn = -1;
    for (int i = 0; i < machine->pageTableSize; i++)
    {
        if(i == vpn) continue;//跳过自己
        if (machine->pageTable[i].valid)
        {
            if (!machine->pageTable[i].dirty)
            {
                ppn = machine->pageTable[i].physicalPage;
                break;
            }
        }
    }
    if (ppn == -1)
    {
        for (int i = 0; i < machine->pageTableSize; i++)
        {
            if(i == vpn) continue;
            if (machine->pageTable[i].valid)
            {
                machine->pageTable[i].valid = FALSE;
                ppn = machine->pageTable[i].physicalPage;

                //写回硬盘
                OpenFile *vm = fileSystem->Open("VirtualMemory");
                vm->WriteAt(&(machine->mainMemory[ppn * PageSize]), PageSize, i * PageSize);
                delete vm; // close file
                break;
            }
        }
    }
    return ppn;
}
```

### 测试

在terminal中输入`.\nachos -d rp -x ../test/halt`可得到结果：

```cpp
vagrant@precise32:/vagrant/nachos/nachos-3.4/code/userprog$ ./nachos -d rp -x ../test/halt
Allocate physical page frame: 0
Bitmap : 000000000000000000000000000001
Allocate physical page frame: 1
Bitmap: 000000000000000000000000000011
Allocate physical page frame: 2
Bitmap: 000000000000000000000000000111
LRU : TLB Hit: 29, Total Visit: 40, TLB Hit Rate: 72.50%
Machine halting!

Ticks: total 28, idle 0, system 10, user 18
```

### 结论

一开始没有页面加载进入内存，之后每需要一页就加载进入内存，成功实现了请求分页。

## 感想&收获

正如您看到的，现在距离ddl还剩19分钟。

本次lab的难度和上一次的相比提升了几个维度，让我明白什么叫做”系统和工程“。我整整做了两个星期，查阅了大量资料，依然还是奋战到了最后一秒才勉强做完。先把作业交了吧，下次补交心得体会和reference。（TOBEDONE）

