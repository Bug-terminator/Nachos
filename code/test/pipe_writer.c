//用用户程序实现pipe，todo

#include "syscall.h"
// #include "../machine/console.h"

#define QUOTE_SIZE 26
char buffer[QUOTE_SIZE];
int pipeID;

int main()
{
    
    buffer[0] = 'a';
    Create("pipe");        //创建管道
    pipeID = Open("pipe"); //打开管道
    // scanf("%s", buffer);               //输出至控制台
    // Write(buffer, QUOTE_SIZE, pipeID); //从管道中读入数据
    Close(pipeID); //关闭管道
    Exit(0);       //退出
}