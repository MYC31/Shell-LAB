#ifndef SYSWRAP_H
#define SYSWRAP_H
#include "unistd.h"



pid_t Fork(void);
pid_t Waitpid(pid_t pid, int *status, int options);
int Execve(const char *file, char *const argv[], char *const envp[]);
int Chdir(const char *file);
int Open(const char *file, int oflag);
void (*Signal(int, void (*)(int)))(int);
int Pipe(int fds[2]);

#endif


