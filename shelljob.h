#ifndef SHELLJOB_H 
#define SHELLJOB_H
#include "sys/wait.h"
#include "shellconf.h"


pid_t doprogram(int argc, char **argv, char **envp, int mode, proc_pipe_info * info);
pid_t dointernal(long type, int argc, char **argv, int mode, proc_pipe_info * info);


#endif
