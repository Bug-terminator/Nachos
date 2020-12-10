//lab7 pipe
// 读者线程从管道中读取数据。
#include "syscall.h"
#define QUOTE_SIZE 40
char buffer[QUOTE_SIZE];
int pipeID;

int main()
{
    pipeID = Open("file2");            //打开管道
    Read(buffer, QUOTE_SIZE, pipeID); //从管道中读入数据
    Close(pipeID);                    //关闭管道
    Exit(0);                          //退出
}