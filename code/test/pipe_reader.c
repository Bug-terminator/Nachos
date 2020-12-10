//用用户程序实现pipe，todo
#include "syscall.h"
#define QUOTE_SIZE 26
char buffer[QUOTE_SIZE];
int pipePtr;

int main()
{
    pipePtr = Open("pipe");            //打开管道
    Read(buffer, QUOTE_SIZE, pipePtr); //从管道中读入数据
    // printf("%s", buffer);               //输出至控制台
    Close(pipePtr);                    //关闭管道
    Exit(0);                            //退出
}