#ifndef SIGNAL_H
#define SIGNAL_H

#include "types.h"

/* Signal numbers */
#define SIGKILL  9
#define SIGTERM 15
#define SIGCHLD 17
#define SIGSTOP 19
#define SIGCONT 18

/* Signal handler type */
typedef void (*signal_handler_t)(int);

/* Signal management */
void signal_init(void);
int signal_send(int pid, int sig);
int signal_handle(int sig, signal_handler_t handler);
void signal_check(void);  /* Called by scheduler */

#endif
