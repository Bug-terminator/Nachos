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
    int i, j;
    int table[N][N];
    for (i = 1; i <= N; ++i)
        for (j = 1; j <= N; ++j)
            table[i - 1][j - 1] = i * j;
    Exit(table[N - 1][N - 1]);
}