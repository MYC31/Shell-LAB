#include "unistd.h"
#include "fcntl.h"
#include "shellconf.h"
#include "signal.h"




/* param: info->queue->pipes, info->queue->pipenum, info->procnum */
void handle_proc_pipe_info(proc_pipe_info * info)
{
    if (info == NULL) return;
    assert(info->queue != NULL);
    int (*pipes)[2] = info->queue->pipes;
    for (int i=0; i < info->queue->pipenum; i++) {
        close(pipes[i][PIPE_READEND]);
        close(pipes[i][PIPE_WRITEEND]);
    }
    free(info->queue);
    free(info);
}




static void handle_background_proc(int mode)
{
    // fprintf(stdout, "hit handle_background_proc");
    // fflush(stdout);
    if (mode == BACK_PROG) {
        /* code for UNIX system */
        int fd = open("/dev/null", O_RDWR, S_IRWXU);
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        close(fd);
        /* UNIX takeover control */
        Signal(SIGCHLD, SIG_IGN);
    }
}



/* simple implementation, no IO redirection or pipeline */
pid_t doprogram(int argc, char **argv, char **envp, int mode, proc_pipe_info * info)
{
    pid_t pid;
    if ((pid = Fork()) == 0) {
        handle_background_proc(mode);
        handle_proc_pipe_info(info);
        if (Execve(argv[0],argv,envp) < 0)
            exit(EXIT_FAILURE);
    } 
    free_proc_pipe_info(info);
    return (mode == BACK_PROG) ? -1 : pid;
}