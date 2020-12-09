#include "syscall.h"
#include "stdio.h"
#define QUOTE_SIZE 26
const char *quote = "Rose is a rose is a rose.";

OpenFileId pipeFile;
int main()
{
    Create("pipe");                     //创建管道
    pipeFile = Open("pipe");            //打开管道
    Write(quote, QUOTE_SIZE, pipeFile); //向管道中写入数据
    Close(pipeFile);                    //关闭管道
    Exit(0);                            //退出
}