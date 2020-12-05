# Lab5 系统调用

todo:搞懂为什么要手动增加pc

## 概述

本实习希望通过修改Nachos系统的底层源代码，达到“实现系统调用”的目标。

## **一、理解Nachos****系统调用**

### Exercise 1  源代码阅读

> 阅读与系统调用相关的源代码，理解系统调用的实现原理。
>
> code/userprog/syscall.h
>
> code/userprog/exception.cc
>
> code/test/start.s

####  code/userprog/syscall.h

> 定义nachos的系统调用,主要包括系统调用号和系统调用函数，内核通过识别用户程序传递的系统调用号确定系统调用类型

```cpp
//定义系统调用号
#define SC_Halt		0
#define SC_Exit		1
#define SC_Exec		2
#define SC_Join		3

#define SC_Create	4
#define SC_Open		5
#define SC_Read		6
#define SC_Write	7
#define SC_Close	8
#define SC_Fork		9
#define SC_Yield	10

//与start.s有关
#ifndef IN_ASM

//停止Nachos，打印统计信息
void Halt();		
 
//地址空间相关，3个:
//用户程序执行完成，status = 0表示正常退出
void Exit(int status);	
//加载并执行名为name的可执行文件，返回其地址空间标识符SpaceId
SpaceId Exec(char *name);
//等待标识符为id的用户线程执行完毕，返回其退出状态
int Join(SpaceId id); 	

//文件系统相关，5个：
//每个打开文件的独立标识
typedef int OpenFileId;	
//创建名为name的文件
void Create(char *name);
//打开文件名为name的文件，返回打开文件的标识符（指针)
OpenFileId Open(char *name);
//从buffer中写size个字节的数据到标识符为id的文件中
void Write(char *buffer, int size, OpenFileId id);
//从标识符为id的文件中读取size个字符到buffer中，返回实际读取的字节数
int Read(char *buffer, int size, OpenFileId id);
//关闭标识符为id的文件
void Close(OpenFileId id);

//用户级线程相关，2个,用于支持多线程用户程序:
//创建和当前线程拥有相同地址空间的线程，运行func指针指向的函数
void Fork(void (*func)());
//当前线程让出CPU
void Yield();		
```

#### code/userprog/exception.cc

> 定义进行异常处理的ExceptionHandler函数，主要流程是根据异常信息处理不同异常，包括系统调用。

在注释中给出了寄存器的作用：

```cpp
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
```

> r2->系统调用返回值
>
> r4/5/6/7->系统调用的四个参数

#### code/test/start.s

辅助用户程序运行的汇编代码，主要包括初始化用户程序和系统调用相关操作

##### 背景知识

> [MIPS中j，jr，jal这三个跳转指令有什么区别？](https://zhidao.baidu.com/question/2116514308751597107.html)
>
> j 跳转
> eg：j 2500 //跳转到目标地址10000，指令中的地址是字地址，所以需要乘以4，转换成字节地址。
> jal 跳转并链接
> eg：jal 2500 //$ra=PC+4, PC=10000，指令中的地址也是字地址，乘以4转换成字节地址。一般用于主程序调用函数时候的跳转，设置函数的返回地址为主程序中跳转指令的下一个指令，意思就是执行完函数就得回到主程序继续执行。寄存器$ra专门用来保存函数的返回地址。
> jr 跳转到寄存器所指的位置
> eg：jr $ra //跳转到寄存器中的地址。一般用于函数执行完返回主函数时候的跳转。

1. 初始化用户程序：通过调用main函数运行用户程序

   ```cpp
   	.globl __start
   	.ent	__start
   __start:
   	jal	main
   	move	$4,$0		
   	jal	Exit	 /* if we return from main, exit(0) */
   	.end __start
   ```

2. 系统调用：将系统调用号存入r2寄存器，跳转到exception.cc，以系统调用Halt为例：

   ```c
   .globl Halt
   	.ent	Halt
   Halt:
   	addiu $2,$0,SC_Halt
   	syscall
   	j	$31
   	.end Halt
   ```

   因为用的是j指令，所以在执行系统调用之后需要人为地将pc的值增加4，否则程序会进入死循环。

#### 系统调用流程：

>machine的Run函数运行用户程序，实现在machine/mipssim.cc，基本流程是通过OneInstruction函数完成指令译码和执行，通过interrupt->OneTick函数使得时钟前进 
>（1）OneInstruction函数判断当前指令是系统调用，转入start.s 
>（2）通过start.s确定系统调用入口，通过寄存器r2传递系统调用号，转入exception.cc（此时系统调用参数位于相应寄存器） 
>（3）exception.cc通过系统调用号识别系统调用，进行相关处理，如果系统调用存在返回值，那么通过寄存器r2传递，流程结束时，需要更新PC <br>（4）系统调用结束，程序继续执行

#### 添加系统调用

> （1）syscfall.h定义系统调用接口、系统调用号 <br>（2）code/test/start.s添加链接代码 <br>（3）exception.cc添加系统调用处理过程 

## **二、文件系统相关的系统调用**

### Exercise 2 系统调用实现

> 类比Halt的实现，完成与文件系统相关的系统调用：Create, Open，Close，Write，Read。Syscall.h文件中有这些系统调用基本说明。
>
> 为此我专门写了一个函数`FileSystemHandler`来处理文件系统系统调用:
>
> ```cpp
> else if (type == SC_Create || type == SC_Open || type == SC_Write || type == SC_Read || type == SC_Close)
>         {
>             // File System System Calls
>             FileSystemHandler(type);
>         }
> ```

advancePC()

> 自增PC

```cpp
void advancePC(void) {
    machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
    machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
    machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
}
```

### Create()

```cpp
void Create(char *name);
```

1. 获取char*地址（r4寄存器）
2. strncpy来获取文件名（可省略）
3. 调用filesys->Create()创建文件
4. 调用advancePC函数自增PC

### Open()

```cpp
OpenFileId Open(char *name);
```

1. 获取char*地址（r4寄存器）
2. strncpy来获取文件名（可省略）
3. 调用filesys->Open()打开文件
4. 将openfile的地址（所谓的OpenFileId）写入r2寄存器
5. 调用advancePC函数自增PC

### Write()

```cpp
void Write(char *buffer, int size, OpenFileId id);
```

1. 读取r4/5/6获取三个参数（初始是int型，强转为对应类型即可）
2. 文件已经打开，通过id强转为openfile*类型即可获得该打开文件
3. 调用openfile->Write()向buffer中写size个字节
4. 调用advancePC函数自增PC

### Read()

```cpp
int Read(char *buffer, int size, OpenFileId id);
```

1. 读取r4/5/6获取三个参数（初始是int型，强转为对应类型即可）
2. 文件已经打开，通过id强转为openfile*类型即可获得该打开文件
3. 调用openfile->Read()从buffer中读取size个字节
4. 将实际读取的字节数写入2号寄存器
5. 调用advancePC函数自增PC

### Close()

```cpp
void Close(OpenFileId id);
```

1. 读取r4获取openFile型指针（初始是int型，强转为对应类型即可）
2. 使用delete析构该openFile
3. 调用advancePC函数自增PC

```cpp
void FileSystemHandler(int type)
{
    if (type == SC_Create)
    {                                                             // void Create(char *name)
        char *name = (char *)machine->ReadRegister(4);
        fileSystem->Create(name, 0);
    }
    else if (type == SC_Open)
    {                                                             // OpenFileId Open(char *name);
        char *name = (char *)machine->ReadRegister(4);
        OpenFile *openFile = fileSystem->Open(name);
        machine->WriteRegister(2, (OpenFileId)openFile);          // return result
    }
    else if (type == SC_Close)
    {                                                              // void Close(OpenFileId id);
        OpenFile *openFile = (OpenFile *)machine->ReadRegister(4); // OpenFile object ptr
        delete openFile;                                           // release the file
    }
    else if (type == SC_Read)
    {                                                  // int Read(char *buffer, int size, OpenFileId id);
        char *buffer = (char *)machine->ReadRegister(4);           // memory starting position
        int size = machine->ReadRegister(5);                       // read "size" bytes
        OpenFile *openFile = (OpenFile *)machine->ReadRegister(6); // OpenFile object ptr
        int numBytes = openFile->Read(buffer, size);
        machine->WriteRegister(numBytes, 2);
    }
    else if (type == SC_Write)
    {                                                  // void Write(char *buffer, int size, OpenFileId id);
        char *buffer = (char *)machine->ReadRegister(4);           // memory starting position
        int size = machine->ReadRegister(5);                       // write "size" bytes
        OpenFile *openFile = (OpenFile *)machine->ReadRegister(6); // OpenFile object ptr
        openFile->Write(buffer, size);
    }
}
```

### Exercise 3  编写用户程序

> 编写并运行用户程序，调用练习2中所写系统调用，测试其正确性。

 

## **三、执行用户程序相关的系统调用**

### Exercise 4 系统调用实现

> 实现如下系统调用：Exec，Fork，Yield，Join，Exit。Syscall.h文件中有这些系统调用基本说明。

### Exercise 5  编写用户程序

> 编写并运行用户程序，调用练习4中所写系统调用，测试其正确性。

