#include "shellconf.h"




/* global pointer to history */
char **         history;
/* crusor of history */
size_t          crusor;
/* capacity of history */
size_t          capacity;



/*
 * initialize a history array 
 * @retval  initialized history.
 */
char **init_history()
{
    crusor = 0;
    capacity = INIT_CAPACITY;
    history = malloc(INIT_CAPACITY*sizeof(char *));
    assert(history != NULL);
    for (size_t i = 0; i < INIT_CAPACITY; i++) {
        history[i] = malloc(MAXLINE*sizeof(char));
    }
    return history;
}


/*
 * Add the command line into the history.
 * @param history   store the command line since the shell started
 * @param cmd       the new command line that needs storage
 * @retval          Pointer to history. 
 *                  NULL if failure, history if success.
 */
char **addhistory(char **history, const char *cmd)
{
    /* history needs expansion */
    if (crusor == capacity) {
        /* expand the history */
        size_t new_capacity = newcapacity(capacity);
        history = realloc(history, new_capacity*sizeof(char *));
        assert(history != NULL);
        for (size_t i=capacity; i < new_capacity; i++)
            history[i] = malloc(MAXLINE*sizeof(char));
        capacity = new_capacity;
    }

    strcpy(history[crusor], cmd);
    crusor++;

    return history;
}