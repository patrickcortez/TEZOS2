#include "signal.h"
#include "task.h"
#include "serial.h"

void signal_init(void) {
    serial_print("[SIGNAL] Signal system initialized\n");
}

int signal_send(int pid, int sig) {
    task_t* task = task_get_by_pid(pid);
    if (!task) return -1;
    
    serial_print("[SIGNAL] Sending signal ");
    serial_print_dec(sig);
    serial_print(" to PID ");
    serial_print_dec(pid);
    serial_print("\n");
    
    /* Handle special signals immediately */
    if (sig == SIGKILL || sig == SIGTERM) {
        /* Terminate task */
        task->state = TASK_ZOMBIE;
        task->exit_code = sig;
        return 0;
    }
    
    /* Add signal to pending queue */
    if (task->pending_signals < MAX_PENDING_SIGNALS) {
        task->signal_queue[task->pending_signals++] = sig;
    }
    
    return 0;
}

int signal_handle(int sig, signal_handler_t handler) {
    task_t* current = task_get_current();
    if (!current) return -1;
    
    if (sig < 0 || sig >= 32) return -1;
    
    /* Install signal handler */
    current->signal_handlers[sig] = handler;
    
    serial_print("[SIGNAL] Installing handler for signal ");
    serial_print_dec(sig);
    serial_print(" in PID ");
    serial_print_dec(current->pid);
    serial_print("\n");
    
    return 0;
}

void signal_check(void) {
    task_t* current = task_get_current();
    if (!current || current->pending_signals == 0) return;
    
    /* Process first pending signal */
    int sig = current->signal_queue[0];
    
    /* Shift queue */
    for (int i = 0; i < current->pending_signals - 1; i++) {
        current->signal_queue[i] = current->signal_queue[i + 1];
    }
    current->pending_signals--;
    
    /* Invoke handler if installed */
    if (current->signal_handlers[sig]) {
        serial_print("[SIGNAL] Invoking handler for signal ");
        serial_print_dec(sig);
        serial_print("\n");
        current->signal_handlers[sig](sig);
    } else {
        /* Default action - terminate */
        serial_print("[SIGNAL] No handler for signal ");
        serial_print_dec(sig);
        serial_print(", terminating\n");
        task_exit(128 + sig);
    }
}
