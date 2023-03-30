#ifndef SHELLCONF_H
#define SHELLCONF_H

#include "stdio.h"
#include "stdlib.h"
#include "syswrap.h"
#include "assert.h"
#include "string.h"



/* prompt for shell */
#define PROMPT              "Shell"

/* cionfiguration for command line */
#define MAX_CMDARG_NUM      128
#define CMD_DLIM            " "
#define PIPE_DLIM           "|"
#define MAXLINE             512

/* signal for background and foreground job*/
#define BACK_PROG           1
#define FORE_PROG           0
#define HAND_JOB            ":)"    /* what's this? :) */


/*
 * judge if the cmd is background job
 * @param cmd   command line input
 * @retval  1 if command is background, 0 otherwise
 */
static int isbackground(const char *cmd) 
{
    size_t len = strlen(cmd);
    return (cmd[len-1]=='&') ? 
            BACK_PROG : FORE_PROG;
}


/*
 * parse the cmd and store args in argv
 * @param cmd   command line input
 * @param argv  buffer to store parsed argv
 * @retval number of args
*/
static int cmdparse(char *cmd, char **argv, const char *dlim) 
{
    char *token = strtok(cmd, dlim);
    int argc = 0;
    while (token != NULL) {
        argv[argc] = malloc(strlen(token));
        strcpy(argv[argc], token);
        token = strtok(NULL, dlim);
        argc++;
    }
    /* added code */
    argv[argc] = NULL;

    return argc;
}  


/* type of hash value */
typedef int                 hash_t;

/* hash value for internal call */
#define CD_HASH             31852
#define HIS_HASH            1884630415
#define EXIT_HASH           2584151
#define MYTOP_HASH          23311534

/* type value for internal call */
/* deprecated!                  */    
#define CD                  0
#define HISTORY             1
#define MYTOP               2
#define EXIT                3


static inline hash_t        hash(char * str);

/*
 * judge if the command is internal job
 * @param argv  parsed command line input
 * @param argc  number of string in argv
 * @retval  1 if process is internal, 0 otherwise
 */
static int isinternal(char ** argv, int argc)
{
    int rval;
    hash_t hashval = hash(argv[0]);
    switch (hashval) {
        case CD_HASH: 
        case HIS_HASH:
        case EXIT_HASH: 
        case MYTOP_HASH:    rval = 1;   break;
        default:            rval = 0;
    }
    return rval;
}


/* value or operation for history */
#define INIT_CAPACITY       2   /* should be 10 */
#define newcapacity(c)      (c << 1)

/* pattern of I/O redirction */
#define REDIR_INPUT         0
#define REDIR_OUTPUT        1
#define REDIR_APPEND        2

/* read and write end of pipe */
#define PIPE_READEND        0
#define PIPE_WRITEEND       1

/* determine the type of I/O redirection */
#define IS_REDIR_SIG(s)     (s[0]=='>' || s[0]=='<' || (s[0]=='>' && s[1]=='>'))


/* struct for I/O redirection */
typedef struct {
    int dir;        /* REDIR_INPUT or REDIR_OUTPUT */
    int sig_idx;    /* index of redir signal in argv */
    int oldfd;
} redir_sig;


/* operation on redir_sig */
static inline hash_t        hash(char * str);


/* hash value for redirection pattern */    
#define OUTPUT_HASH         3491
#define INPUT_HASH          3489
#define APPEND_HASH         31481


/* data structure for handling process collection */
typedef struct {
    int mode;
    pid_t pid;
} task_struct;

// #define VALID_PID(task) ((task)->pid > 0)

typedef struct {
    int tasknum;
    task_struct **tasks;
} task_queue;

#define VALID_PID(queue,i)  ((queue)->tasks[(i)]->pid > 0)
#define IS_FOREJOB(queue,i) ((queue)->tasks[(i)]->mode == FORE_PROG)


/*
 * compute hash value of str
 * @param str   input string
 * @retval  hash value of str
 */
static inline hash_t hash(char * str) 
{
    size_t hash = 381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 3) + hash) + c;
    }
    // printf("%lu\n", hash);
    return hash;
}


static task_struct * task_init(int mode, pid_t pid) 
{
    task_struct * task = malloc(sizeof(task_struct));
    task->mode = mode; task->pid = pid;
    return task;
}


static task_queue * task_queue_init(int tasknum) 
{
    task_queue * queue = malloc(sizeof(task_queue));
    queue->tasknum = tasknum;
    queue->tasks = malloc(tasknum*sizeof(task_struct *));
    return queue;
}



inline static void collect_single_task(task_struct * task) 
{
    if (task == NULL) return;
    if (task->mode == FORE_PROG) {
        Waitpid(task->pid, NULL, 0);
    }
    free(task);
}



static void collect_all_task(task_queue * queue)
{
    if (queue == NULL) return ;
    int i;
    for (i = 0; i < queue->tasknum; i++) {
        Waitpid(queue->tasks[i]->pid, NULL, 0);
    }
}


/* struct for handling closure of pipe in child process */
/* I start to realize that i might have made this thing complicate :< */
typedef struct {
    int (*pipes)[2];
    int pipenum;
} pipe_queue;


static pipe_queue * pipe_queue_init(int pipes[][2], int pipenum)
{
    pipe_queue * queue = malloc(sizeof(pipe_queue));
    assert(queue != NULL);
    queue->pipes = pipes; 
    queue->pipenum = pipenum;
    return queue;
}

typedef struct {
    pipe_queue * queue;
    int procnum;
} proc_pipe_info;


static proc_pipe_info * proc_pipe_info_init(int pipes[][2], int pipenum, int procnum)
{
    proc_pipe_info * info = malloc(sizeof(proc_pipe_info));
    assert(info != NULL);
    info->queue = pipe_queue_init(pipes, pipenum);
    info->procnum = procnum;
    return info;
}


static void free_proc_pipe_info(proc_pipe_info * info)
{
    if (info == NULL) return;
    if (info->queue != NULL) 
        free(info->queue);
    free(info);
}


typedef struct {
    int pid;
    int ticks;
} proc_tick;


#endif

