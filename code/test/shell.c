#include "syscall.h"
// Parsed command representation
#define EXEC 1
#define REDIR 2
#define PIPE 3
#define LIST 4
#define BACK 5
#define NULL 0

#define MAXARGS 10
#define NUMSPACE 5
#define NUMSYMBOLS 4

char whitespace[NUMSPACE];
char symbols[NUMSYMBOLS];
char argv[MAXARGS];

int IsPipe(char *argv[])
{
    int i;
    i = 0;
    while (argv[i] != 0)
    {
        if (argv[i] == "|")
        {
            return i + 1;
        }
        ++i;
    }
    return 0;
}
void ParsePipe(char *input[], char *output1[], char *output2[]) //用于将input按照|进行切分，最后一个后面为当前路径    {
{
    int i, size1, size2, ret, j;
    i = 0;
    size1 = 0;
    size2 = 0;
    ret = IsPipe(input); 

    while (argv[i] == "|")
    {
        output1[size1++] = input[i++];
    }
    output1[size1] = 0;

    j = ret; 
    while (input[j] != NULL)
    {
        output2[size2++] = input[j++];
    }
    output2[size2] = NULL;
}
struct cmd
{
    int type;
};

struct execcmd
{
    int type;
    char *argv[MAXARGS];
    char *eargv[MAXARGS];
};

struct redircmd
{
    int type;
    struct cmd *cmd;
    char *file;
    char *efile;
    int mode;
    int fd;
};

struct pipecmd
{
    int type;
    struct cmd *left;
    struct cmd *right;
};

struct listcmd
{
    int type;
    struct cmd *left;
    struct cmd *right;
};

struct backcmd
{
    int type;
    struct cmd *cmd;
};

struct cmd *
nulterminate(struct cmd *cmd)
{
    int i;
    struct backcmd *bcmd;
    struct execcmd *ecmd;
    struct listcmd *lcmd;
    struct pipecmd *pcmd;
    struct redircmd *rcmd;

    if (cmd == 0)
        return 0;

    switch (cmd->type)
    {
    case EXEC:
        ecmd = (struct execcmd *)cmd;
        for (i = 0; ecmd->argv[i]; i++)
            *ecmd->eargv[i] = 0;
        break;

    case REDIR:
        rcmd = (struct redircmd *)cmd;
        nulterminate(rcmd->cmd);
        *rcmd->efile = 0;
        break;

    case PIPE:
        pcmd = (struct pipecmd *)cmd;
        nulterminate(pcmd->left);
        nulterminate(pcmd->right);
        break;

    case LIST:
        lcmd = (struct listcmd *)cmd;
        nulterminate(lcmd->left);
        nulterminate(lcmd->right);
        break;

    case BACK:
        bcmd = (struct backcmd *)cmd;
        nulterminate(bcmd->cmd);
        break;
    }
    return cmd;
}

int gettoken(char **ps, char *es, char **q, char **eq)
{
    char *s;
    int ret;

    s = *ps;
    if (q)
        *q = s;
    ret = *s;
    switch (*s)
    {
    case 0:
        break;
    case '|':
    case '<':
        s++;
        break;
    case '>':
        s++;
        if (*s == '>')
        {
            ret = '+';
            s++;
        }
        break;
    default:
        ret = 'a';
        break;
    }
    if (eq)
        *eq = s;

    *ps = s;
    return ret;
}

int main()
{

    SpaceId newProc;
    OpenFileId input = ConsoleInput;
    OpenFileId output = ConsoleOutput;
    char ch, buffer[60];
    int i;
    whitespace[0] = '\t';
    whitespace[1] = '\r';
    whitespace[2] = '\n';
    whitespace[3] = '\v';
    symbols[0] = '<';
    symbols[1] = '|';
    symbols[2] = '>';
    while (1)
    {
        Cmd("echo -n 'litang@Nachos$ '");
        i = 0;
        do
        {
            Read(&buffer[i], 1, input);
        } while (buffer[i++] != '\n');
        buffer[--i] = '\0';
        if (buffer[0] == 'q' && buffer[1] == '\0')
            Exit(0);
        if (i > 0)
            Cmd(buffer);
    }
}
