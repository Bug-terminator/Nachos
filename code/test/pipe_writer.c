//lab7 pipe
//写者线程负责创建管道，并向管道中写入数据；然后调用Exec()执行读者线程
// #include "syscall.h" //有名管道
// #define QUOTE_LEN 40    //诗的长度
// int fileID1, fileID2;   //两个打开文件的id
// int numBytes;           //实际读取的字符数
// char buffer[QUOTE_LEN]; //缓冲区
// int main()
// {
//     Create("file2");                             //创建管道
//     fileID1 = Open("file1");                     //打开文件file1
//     fileID2 = Open("file2");                     //打开管道file2
//     numBytes = Read(buffer, QUOTE_LEN, fileID1); //从file1中读取字节到buffer中
//     Write(buffer, numBytes, fileID2);            //再从buffer中写入file2中
//     Exec("../test/pipe_reader");                 //执行读者线程
//     Close(fileID1);                              //关闭两个文件
//     Close(fileID2);
//     Exit(0);
// }

#include "syscall.h"    //匿名管道
#define QUOTE_LEN 40    //诗的长度
char buffer[QUOTE_LEN]; //缓冲区

void child()
{
    int dummy;                      //dummy继承自父亲,但是需要声明一下，否则会报错
    Read(buffer, QUOTE_LEN, dummy); //从管道中读入数据
    Close(dummy);                   //关闭文件
    Exit(0);
}

int main()
{
    int fileID1, fileID2, dummy;                 //两个打开文件的id和一个dummy变量，用于继承父线程的打开文件
    int numBytes;                                //实际读取的字符数
    Create("file2");                             //创建管道
    fileID1 = Open("file1");                     //打开文件file1
    fileID2 = Open("file2");                     //打开管道file2
    dummy = fileID2;                             //为dummy赋值，Nachos只支持这种笨拙的方式
    numBytes = Read(buffer, QUOTE_LEN, fileID1); //从file1中读取字节到buffer中
    Write(buffer, numBytes, fileID2);            //再从buffer中写入file2中
    Fork(child);                                 //执行读者线程
    Close(fileID1);
    Exit(0);
}