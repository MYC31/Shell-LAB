#ifndef PIPEHANDLER_H
#define PIPEHANDLER_H

#include "shellconf.h"

/* param: info->queue->pipes, info->queue->pipenum, info->procnum */
static void handle_proc_pipe_info(proc_pipe_info * info)
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

#endif