#include "shelljob.h"
#include "unistd.h"
#include "fcntl.h"
#include "history.h"
#include "sys/stat.h"



/* global pointer to history */
extern char **         history;
extern size_t          crusor;




/*
 * check if the command line contains I/O redirection symbols
 * @param argc      number of args in argv
 * @param argv      parsed command line input
 * @retval          1 with I/O redirection, 0 otherwise
 */
static int needredir(int argc, char **argv) 
{
    int i;
    for (i=0; i < argc; i++)
        if (IS_REDIR_SIG(argv[i])) break;
    return (i==argc) ? -1 : i;
}



/*
 * correctly set I/O redirection for the programme
 * @param argc      number of args in argv
 * @param argv      parsed command line input
 * @retval          a struct redir_sig including info about the redirection   
 */
static redir_sig * setredir(int argc, char **argv)
{
    redir_sig *sig = NULL;
    int i = needredir(argc, argv);

    if (i >= 0) 
    {
        sig = malloc(sizeof(redir_sig));
        assert(sig != NULL);
        sig->sig_idx = i;
        hash_t hashval = hash(argv[i]);
        int newfd = open(argv[i+1], O_RDWR|O_CREAT);
        switch (hashval) {
            case INPUT_HASH:
            {
                sig->dir = REDIR_INPUT;
                sig->oldfd = dup(STDIN_FILENO);
                chmod(argv[i+1], S_IREAD|S_IWRITE);
                dup2(newfd, STDIN_FILENO);
                break;
            }
            case OUTPUT_HASH:
            {
                sig->dir = REDIR_OUTPUT;
                sig->oldfd = dup(STDOUT_FILENO);
                chmod(argv[i+1], S_IREAD|S_IWRITE);
                lseek(newfd, 0, SEEK_SET);
                dup2(newfd, STDOUT_FILENO);
                break;
            }
            case APPEND_HASH: 
            {
                sig->dir = REDIR_APPEND;
                sig->oldfd = dup(STDOUT_FILENO);
                chmod(argv[i+1], S_IREAD|S_IWRITE);
                dup2(newfd, STDOUT_FILENO);
                break;
            }
            default:
            {
                fprintf(stderr,"invalid parameter for I/O redirection!\n");
                fflush(stderr);
            }
        }
        close(newfd);
    }
    return sig;
}



/* 
 * correctly unset I/O redirection for the programme
 * @param sig       a struct redir_sig including info about the redirection        
 */
static void unsetredir(redir_sig *sig)
{
    if (sig != NULL) {
        switch (sig->dir) 
        {
            case REDIR_INPUT:
                dup2(sig->oldfd, STDIN_FILENO);
                close(sig->oldfd);
                break;
            case REDIR_OUTPUT: 
            case REDIR_APPEND: 
                dup2(sig->oldfd, STDOUT_FILENO);
                close(sig->oldfd);
                break;
        }
        free(sig);
    }
}



/* not considering the collision between pipe and IO redirection */
static task_struct * run_single_process(char *cmdline, char **envp, proc_pipe_info * info) /* cmdline is a copy of original command line */
{
    char cmdline_cpy[MAXLINE];
    strcpy(cmdline_cpy, cmdline);
    char *buff[MAX_CMDARG_NUM];
    int argnum = cmdparse(cmdline, (char **)buff, CMD_DLIM);
    int jobtype = isinternal(buff, argnum);
    int mode = isbackground(cmdline_cpy);

    /* delete the last '&' in cmd of background job */
    if (mode == BACK_PROG) {
        free(buff[argnum-1]);
        buff[argnum-1] = NULL;
        argnum--;
    }
    redir_sig * sig = setredir(argnum, buff);
    /* delete the redundent arg in buff */
    if (sig != NULL) {
        for (int i=sig->sig_idx; i < argnum; i++) {
            free(buff[i]); 
            buff[i] = NULL;
        }
        argnum = sig->sig_idx+1;
    }

    /* execute the job */
    pid_t pid;
    task_struct * task;
    if (jobtype == 1) { /* internal case */
        /* let dointernal support info parameter later... */
        pid = dointernal(hash(buff[0]), argnum, buff, mode, info);
        task = (info == NULL) ? NULL : task_init(mode, pid);
    } else {
        pid = doprogram(argnum, buff, envp, mode, info);
        task = task_init(mode, pid);
    }

    /* unset potential I/O redirection */
    unsetredir(sig);

    /* free buff for next job */
    for (int i=0; i < argnum; i++) {
        free(buff[i]);
    }

    return task;
}






/* pipe process is identified as background job */
/* beaware to close all the duplicate pipes in process group, because as lon as a fd is pointed to a file/pipe, 
    the OS will keep that file open and the process that read from that fd will not read an EOF, thus keeping running and 
    refusing to be collected. */
/* consider stripping run_pipe_process from run_single_process */
/* param: process_num, buff, envp */
static task_queue * run_pipe_process(int argc, char **buff, char **envp)
{
    task_queue * queue = task_queue_init(argc);
    int pipenum = argc-1;
    // int **pipes = malloc(2*pipenum*sizeof(int));
    int pipes[pipenum][2];
    for (int i=0; i < pipenum; i++)
        Pipe(pipes[i]);

    /* save stdin and stdout */
    int stdin_fd  = dup(STDIN_FILENO);
    int stdout_fd = dup(STDOUT_FILENO);

    proc_pipe_info *info;
    /* initialize the first pipe process */
    dup2(pipes[0][PIPE_WRITEEND], STDOUT_FILENO);
    info = proc_pipe_info_init(pipes, pipenum, 0);
    queue->tasks[0] = run_single_process(buff[0], envp, info);

    int pid_idx;
    for (pid_idx=1; pid_idx < argc-1; pid_idx++) {
        dup2(pipes[pid_idx-1][PIPE_READEND], STDIN_FILENO);
        dup2(pipes[pid_idx][PIPE_WRITEEND], STDOUT_FILENO);
        info = proc_pipe_info_init(pipes, pipenum, pid_idx);
        queue->tasks[pid_idx] = run_single_process(buff[pid_idx], envp, info);
    }
    /* initialize the last pipe process*/
    dup2(pipes[pipenum-1][PIPE_READEND], STDIN_FILENO);
    dup2(stdout_fd, STDOUT_FILENO);
    info = proc_pipe_info_init(pipes, pipenum, argc-1);
    queue->tasks[argc-1] = run_single_process(buff[argc-1], envp, info);

    for (int i=0; i < pipenum; i++) {
        close(pipes[i][PIPE_READEND]);
        close(pipes[i][PIPE_WRITEEND]);
    }
    /* retrieve stdin file descriptor */
    dup2(stdin_fd, STDIN_FILENO);
    /* close the saved stdin and stdout file descriptor */
    close(stdin_fd);
    close(stdout_fd);

    return queue;
}



/* stucked in the loop, cannot collect the "grep" process!!! */
/* code is right, but program "grep" somehow fails to exit */
static void wait_task_queue(task_queue * queue)
{
    if (queue == NULL) return ;
    int i;
    for (i = 0; i < queue->tasknum; i++) {
        if (queue->tasks[i] == NULL) continue;
        if (VALID_PID(queue,i) && IS_FOREJOB(queue,i)) {
            pid_t pid = Waitpid(queue->tasks[i]->pid, NULL, 0);
            /* test code */
            // fprintf(stdout, "[%d]th: P[%d] has been collected\n", i, pid);
        }
        // fflush(stdout);
        free(queue->tasks[i]);
    }
    free(queue);
}







int main(int argc, char **argv, char **envp)
{
    /* command line buffer */
    char cmdline[MAXLINE];

    /* initialize buffer for history job */
    history = init_history();

    /* working directory used for shell prompt */
    char workdir[MAXLINE];

    while (1) {
        getcwd(workdir, MAXLINE);
        fprintf(stdout, "%s > ", workdir); fflush(stdout);
        fgets(cmdline, MAXLINE, stdin);
        
        if (strcmp(cmdline, "\n") == 0) continue;

        /* update the history, with the last '\n' unremoved */
        history = addhistory(history, cmdline);

        /* remove the last '\n' of cmdline */
        size_t len = strlen(cmdline);
        cmdline[len-1] = '\0';

        /* copy the command line */
        char cmdline_cpy[MAXLINE];
        strcpy(cmdline_cpy, cmdline);

        /* initial parse of command line needed here */
        char *buff[MAX_CMDARG_NUM];

        /* parse the potential pipe process */
        int process_num = cmdparse(cmdline_cpy, buff, PIPE_DLIM);

        /* logic error, should fork for every process in pipe */
        if (process_num <= 0) {
            fprintf(stdout, "command parse failed!\n");
            fflush(stdout);
        } else if (process_num == 1) {
            task_struct * task = run_single_process(buff[0], envp, NULL);
            collect_single_task(task);
        } else { /* more than 2 processes and pipe needed */
            task_queue * queue = run_pipe_process(process_num, buff, envp);
            wait_task_queue(queue);
        }
        
    }

    exit(EXIT_SUCCESS);
}



