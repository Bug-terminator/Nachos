# Lab5 系统调用

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



## **二、文件系统相关的系统调用**

### Exercise 2 系统调用实现

> 类比Halt的实现，完成与文件系统相关的系统调用：Create, Open，Close，Write，Read。Syscall.h文件中有这些系统调用基本说明。

### Exercise 3  编写用户程序

> 编写并运行用户程序，调用练习2中所写系统调用，测试其正确性。

 

## **三、执行用户程序相关的系统调用**

### Exercise 4 系统调用实现

> 实现如下系统调用：Exec，Fork，Yield，Join，Exit。Syscall.h文件中有这些系统调用基本说明。

### Exercise 5  编写用户程序

> 编写并运行用户程序，调用练习4中所写系统调用，测试其正确性。