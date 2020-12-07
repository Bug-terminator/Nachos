#include "syscall.h"
int spaceID;
void func()
{
    Create("text1");
    Exit(0);
}

int main()
{
    Fork(func);
    Yield();
    spaceID = Exec("../test/file_syscall_test");
    // Join(spaceID);
    Exit(0);
}