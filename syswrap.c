#include "unistd.h"
#include "stdio.h"
#include "fcntl.h"
#include "sys/wait.h"
#include "stdarg.h"
#include "signal.h"
#include "string.h"


static void err_sys(const char *err)
{
    fprintf(stdout,"%s",err);
    fflush(stdout);
}

pid_t Fork(void)
{
    pid_t pid;
    if ((pid = fork()) < 0)
        err_sys("fork failed!\n");
    return pid;
}

pid_t Waitpid(pid_t pid, int *status, int options)
{
    pid_t p;
    if ((p = waitpid(pid, status, options)) < 0)
        err_sys("waitpid failed!\n");
    return p;
}

int Execve(const char *file, char *const argv[], char *const envp[])
{
    int rval;
    if ((rval = execve(file, argv, envp)) < 0) {
        char file_cp[128];
        strcpy(file_cp, file);
        err_sys(strcat(file_cp, ": not found\n"));
    }
    return rval;
}

int Chdir(const char *file)
{
    int rval;
    if ((rval = chdir(file)) < 0)
        err_sys("chdir failed!\n");
    return rval;
}

int Open(const char *file, int oflag)
{
    int fd;
    if ((fd = open(file, oflag)) < 0)
        err_sys("open failed!\n");
    return fd;
}

void (*Signal(int signum, void (*func)(int)))(int)
{
    void (*fp)(int);
    if ((fp = signal(signum, func)) == SIG_ERR)
        err_sys("signal failed!\n");
    return fp;
}

int Pipe(int *fds)
{
    int rval;
    if ((rval = pipe(fds)) < 0)
        err_sys("pipe failed!\n");
    return rval;
}