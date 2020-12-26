#include "syscall.h"
#define QUOTE_LEN 40    //诗的长度
int fileID1, fileID2;   //两个打开文件的id
int numBytes;           //实际读取的字符数
char buffer[QUOTE_LEN]; //缓冲区
int main()
{
    Create("file2");                             //创建文件
    fileID1 = Open("file1");                     //打开文件file1
    fileID2 = Open("file2");                     //打开文件file2
    // numBytes = Read(buffer, QUOTE_LEN, fileID1); //从file1中读取字节到buffer中
    // Write(buffer, numBytes, fileID2);            //再从buffer中写入file2中
    Close(fileID1);                              //关闭两个文件
    Close(fileID2);
    Exit(0);
}