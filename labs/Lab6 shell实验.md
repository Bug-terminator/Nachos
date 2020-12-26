# lab6 shell

> 本次实习需要实现用户程序shell。shell是提供使用者使用界面的软件（命令解析器），他接收用户命令，然后调用相应的应用程序。本次shell实现的基础是前面已经完成的相关功能。注意到nachos已经实现简单的shell，我们只需要在此基础上进行相关的修改。

*shell*的本质就是一个无限循环的用户程序，其执行流程可以用以下伪代码表示：

```cpp
int main()
{
  while(true)
  {
    //获取指令
    //调用相应处理程序(先fork,然后exec)
  }
}
```

很简单对吗？我们的任务就是实现相应的系统调用。

本次*lab*我将实现几个常用的*linux*指令：

|  命令   |     功能     |                       实现                       |
| :-----: | :----------: | :----------------------------------------------: |
|  *ls*   | 列出当前目录 |     打开*cwd*指向的目录，遍历其中元素并打印      |
|  *pwd*  | 打印当前目录 |             打印进程中的*cwd*变量。              |
|  *cd*   |   切换目录   |  调用*chdir*函数切换到目标目录，并维护*cwd*变量  |
| *mkdir* |   创建目录   |         调用*create*创建类型为目录的对象         |
| *touch* |   创建文件   |         调用*create*创建类型为文件的对象         |
|  *rm*   |   删除文件   |                 调用*remove*函数                 |
| *rmdir* |   删除目录   |                  调用*rm* -*rf*                  |
| *exit*  |     退出     |         调用*lab5*中实现的*Exit*系统调用         |
| *echo*  |  打印字符串  |                 调用*printf*函数                 |
|  *cat*  | 打印文件内容 | 和*echo*类似，额外需要从文件中读入*buffer*的操作 |

添加系统调用流程：

1. *syscall.h*中定义系统调用接口、系统调用号；
2. *start.s*中添加链接代码；
3. *exception.cc*中增加系统调用的处理函数。

本次试验基于*Unix*文件系统，因为*Nachos*还是太过于简陋，要用*Nachos*实现相应的内容(*todo*)。

## Improve

管道/重定向

用过*Linux*的人都知道，管道和文件重定向才是*Linux*的灵魂，正是这两个巧妙的设计，使得程序不需要知道是谁传入的指令，也不需要知道结果将输出给谁。它们只需要从自己的打开文件*0*中读入参数，将自己的工作完成，再把结果写入文件*1*中。这种对文件的精妙抽象，使得*linux*把很多东西变得简单。

另外*fork*和*exec*的分离，也使得在二者中间实现文件重定向变为可能。

#### 让Shell支持管道

##### 管道的实现方法

1.管道”|”是将”|”左边指令的执行结果作为”|”右边的输入。
2.我们可以将一条带有”|”的指令进行分割，将管道左边指令的输出结果重定向到管道的输入（也就是管道的写端），将标准输入重定向至管道的读端。让两个进程分别执行两边重定向后的指令。

##### 管道的具体实现

（1）首先判断一条指令是否包含管道（指令已经存放至char*的数组里面，且该数组以NULL作为结束标志）

```
int IsPipe(char* argv[])
{                                                                                                                                       
    int i = 0;
    while(argv[i] != NULL)
    {
        if(strcmp(argv[i], "|") == 0)
        {
            return i+1;     //i的位置是管道，则i+1就是管道的下一个命令
        }
        ++i;
    }
    return 0;
}
```

（2）将管道左右两边的指令进行切分,切分后的分别放入两个char*数组里，并保证每个数组以NULL作为结束标志

```
void ParsePipe(char* input[], char* output1[],char* output2[])//用于将input按照|进行切分，最后一个后面为当前路径    {
    int i = 0;
    int size1 = 0;
    int size2 = 0;
    int ret = IsPipe(input);   //ret是input数组中管道"|"的下一个位置

    while(strcmp(input[i], "|")!=0)
    {
        output1[size1++] = input[i++];
    }
    output1[size1] = NULL;  //将分割出来的两个char*数组都以NULL结尾

    int j = ret;//j指向管道的后面那个字符
    while(input[j] != NULL)
    {
        output2[size2++] = input[j++];
    }           
    output2[size2] = NULL;
}
```

（3）执行包含管道的指令（首先创建一个进程，让子进程创建一个管道，再创让子进程建一个子进程（孙子进程），让孙子进程执行管道左边的指令，并将输出结果重定向入管道，让子进程从执行管道右边的指令）

## 测试

我仿照*Linux*把*prompt*的内容改为：*litang@Nachos$*。

编译并运行*shell*：

```shell
vagrant@precise32:/vagrant/nachos/nachos-3.4/code/lab6$ ../code/userprog/./nachos -x ../code/test/shell
litang@Nachos$ pwd
/vagrant/nachos/nachos-3.4/code/lab6/dir1
litang@Nachos$ ls
litang@Nachos$ mkdir dir1
litang@Nachos$ touch dir1/file1
litang@Nachos$ ls
dir1
litang@Nachos$ cd dir1
litang@Nachos$ pwd
/vagrant/nachos/nachos-3.4/code/lab6/dir1
litang@Nachos$ ls
file1
litang@Nachos$ cd ..
litang@Nachos$ pwd
/vagrant/nachos/nachos-3.4/code/lab6
litang@Nachos$ ls
dir1
litang@Nachos$ rmdir dir1
litang@Nachos$ ls
litang@Nachos$ echo hello world
hello world
litang@Nachos$ echo hello world > file1
litang@Nachos$ cat file1
hello world
litang@Nachos$ ls
file1
litang@Nachos$ ls | cat
file1
```

## 结论

成功实现*shell*！

## 课程建议

本学期的*Nachos*之旅到此就告一段落了，回想这一路，痛是真的：我经历了一开始装环境的折磨，还有内存管理和文件系统两个*lab*的地狱级难度，毫不夸张地说，这学期我基本把所有的时间都给了*Nachos*。但是我的成长也是真的。作为一个跨考的学生，一开始，我连源代码都读不懂，尤其是*Nachos*模拟中断那部分。如果把*Nachos*比作一个项目的话，那么上手这个项目，我花了近一个月。同时我也参考了大量前人做出的实验结果，其中对我帮助最大的还是*github*上的李大为学长，他不仅在完成了大部分实验内容的前提下，还能自定义*nachos* *Debug*的颜色（这点在突显了他的嵌入式基础），最重要的是通过各种*Debug*符号巧妙控制调试信息（一开始我真的没懂*Debug*函数是干嘛用的，这点反映出他对源代码的理解），是他能够把自己做的*Nachos*开源出来，才得以让我能够站在巨人的肩膀上更进一步，我在此表示诚挚的谢意。

