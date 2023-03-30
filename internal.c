/*
 * implementation of dointernal
 */
#include "sys/wait.h"
#include "fcntl.h"
#include "pipehandler.h"
#include "history.h"
#include "signal.h"
#include "dirent.h"



/* pointer to history in shell.c */
extern char **      history;
/* crusor of history in shell.c */
extern size_t       crusor;



#define MIN(n,m) ((n < m) ? n : m)

#define INORDER_HISTORY             0
#define REVERSE_HISTORY             1

#define HISTORY_MODE(n)             ((n)==crusor ? INORDER_HISTORY : REVERSE_HISTORY)
#define HISTORY_START(n)            (crusor-(n))

#define PAGE_SIZE(argv)             (atoi(argv[0]))
#define TOTAL_PAGE(argv)            (atoi(argv[1]))
#define FREE_PAGE(argv)             (atoi(argv[2]))
#define LARGESTP_AGE(argv)          (atoi(argv[3]))
#define CACHED_PAGE(argv)           (atoi(argv[4]))

#define KB_SIZE                     1024
#define TOTAL_MEMORY_SIZE(argv)     ((PAGE_SIZE(argv)*TOTAL_PAGE(argv)) /KB_SIZE)
#define FREE_MEMORY_SIZE(argv)      ((PAGE_SIZE(argv)*FREE_PAGE(argv))  /KB_SIZE)
#define CACHED_MEMORY_SIZE(argv)    ((PAGE_SIZE(argv)*CACHED_PAGE(argv))/KB_SIZE)     

#define PROC_DIR                    "/proc/"
#define VALID_PROC_DIR(d)           (atoi(d))
#define TICK_INDEX                  7
#define MAX_PROG                    10240



static void cd(int argc, char **argv);
static void phistory(int argc, char **argv);
static void mytop(int argc, char **argv);
static void myExit(int argc, char **argv);




/* param: info->queue->pipes, info->queue->pipenum, info->procnum */
extern void handle_proc_pipe_info(proc_pipe_info * info);




/*
 * core function that implements internal command
 * @type            specify the type among four ones
 * @param argc      number of arg in argv
 * @param argv      string array of parsed command line
 * @retval          possible pid of background processs   
 */
pid_t dointernal(long type, int argc, char **argv, int mode, proc_pipe_info * info)
{
    pid_t pid;
    void (*func)(int,char **);
    
    /* set the correct function pointer */
    switch (type) {
        case CD_HASH:       func = cd;          break;
        case HIS_HASH:      func = phistory;    break;
        case EXIT_HASH:     func = myExit;      break;
        case MYTOP_HASH:    func = mytop;       break;
        default:            func = NULL;
    }
    assert(func != NULL);

    if (mode == FORE_PROG) {
        if (info == NULL) {
            func(argc, argv);
            pid = -1;
        } else {
            pid = Fork();
            if (pid == 0) {
                handle_proc_pipe_info(info);
                func(argc, argv);
                exit(EXIT_SUCCESS);
            }
            // free(info->queue);
            // free(info);
        }
    } else if (mode == BACK_PROG) {
        pid = Fork();
        if (pid == 0) {
            /* code for UNIX system */
            int fd = open("/dev/null", O_RDWR, S_IRWXU);
            dup2(fd, STDIN_FILENO);
            dup2(fd, STDOUT_FILENO);
            Signal(SIGCHLD, SIG_IGN);
            func(argc, argv);
            exit(EXIT_SUCCESS);
        } else {
            /* no collection needed for background process */
            pid = -1;
        }
    } else {
        fprintf(stdout, "error in dointernal!\n");
        fflush(stdout);
    }
    return pid;
}



/*
 * implementation of changing directory
 * @param argc      number of arg in argv
 * @param argv      string array of parsed command line
 */
static void cd(int argc, char **argv)
{
    Chdir(argv[1]);
    /* test code */
    static const int maxline = 100;
    char buff[maxline];
    getcwd(buff, maxline);
    fprintf(stdout,"current dir = %s\n",buff);
    fflush(stdout);
}




inline static int history_num(char *arg)
{
    if (arg == NULL) return crusor;
    int argval = atoi(arg);
    int rval;
    if (argval < 0) rval = 0;
    else if(argval >= 0 && argval <= crusor) rval = argval;
    else rval = crusor;
    return rval;
}



/*
 *   bug in history
 */
static void phistory(int argc, char **argv)
{
    assert(history !=  NULL);
    char *arg = (argc > 1) ? argv[1] : NULL;
    int num = history_num(arg);
    int mode = HISTORY_MODE(num);
    int index = HISTORY_START(num);
    for ( ; index < crusor; index++) {
        fprintf(stdout, "%s", history[index]);
    }
    fflush(stdout);
} 


/* test code */
/*
static void checkargv(int argc, char **argv)
{
    fprintf(stdout, "parsed meminfo: ");
    for (int i=0; i < argc; i++) {
        fprintf(stdout, "|%d| ", atoi(argv[i]));
    }
    fprintf(stdout, "\n");
    fflush(stdout);
}
*/

inline static void print_meminfo(int total, int free, int cached)
{
    fprintf(stdout, "total:%d free:%d cached:%d\n", total, free, cached);
    fflush(stdout);
}



static void mytop(int argc, char **argv)
{
    /* part 1 */
    int meminfo = Open("/proc/meminfo", O_RDONLY);
    char *cmdline = malloc(MAXLINE*sizeof(char));
    char *buff = malloc(MAXLINE*sizeof(char));
    while (read(meminfo, buff, MAXLINE) > 0) {
        strcat(cmdline, buff);
    }
    free(buff);
    close(meminfo);

    cmdline[strlen(cmdline)-1] = '\0';
    char **argarr = malloc(MAX_CMDARG_NUM*sizeof(char *));
    int argnum = cmdparse(cmdline, argarr, " ");
    free(cmdline);
    int total_size = TOTAL_MEMORY_SIZE(argarr);
    int free_memsize = FREE_MEMORY_SIZE(argarr);
    int cached_memsize = CACHED_MEMORY_SIZE(argarr);
    print_meminfo(total_size, free_memsize, cached_memsize);

    /* part 2 */
    DIR *proc_dir = opendir(PROC_DIR);
    if (proc_dir == NULL) {
        fprintf(stdout, "cannot open \"/proc\" directory\n");
        fflush(stdout);
        return ;
    }
    struct dirent *dir;
    int total_ticks = 0;
    proc_tick *procs[MAX_PROG];
    int procs_idx = 0;
    while ((dir = readdir(proc_dir)) != NULL) {
        const char *dir_name = dir->d_name;
        int pid;
        if ((pid = VALID_PROC_DIR(dir_name)) > 0) {
            char dirbuff[MAXLINE];
            strcpy(dirbuff, PROC_DIR);
            strcat(dirbuff, dir_name);
            strcat(dirbuff, "/psinfo");
            int fd = open(dirbuff, O_RDONLY);
            char procbuff[MAXLINE];
            int offset = 0;
            int bytes = 0;
            while ((bytes = read(fd, procbuff+offset, MAXLINE)) > 0) {
                offset += bytes;
            }
            close(fd);
            /* procbuff now copntains psinfo */
            char *args[MAX_CMDARG_NUM];
            cmdparse(procbuff, args, " ");
            int _tick = atoi(args[TICK_INDEX]);
            total_ticks += _tick;
            proc_tick *tick = malloc(sizeof(proc_tick));
            tick->pid = pid; tick->ticks = _tick;
            procs[procs_idx] = tick;
            procs_idx++;
        }
    }
    /* no bug until here */
    for (int i=0; i < procs_idx; i++) {
        int pid = procs[i]->pid;
        double percentage = procs[i]->ticks*1.0 / total_ticks * 100;
        fprintf(stdout, "[%3d]: %2f%%\n", pid, percentage);
    }
    fflush(stdout);
    for (int i=0; i < procs_idx; i++) {
        free(procs[i]);
    }
}



/*
 * exit the shell
 * @param argc      number of arg in argv
 * @param argv      string array of parsed command line
 */
static void myExit(int argc, char **argv)
{
    exit(EXIT_SUCCESS);
}





