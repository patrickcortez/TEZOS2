#include "libc.h"

#define MAX_ARGS 10

void split_command(char* cmd, char** args) {
    int i = 0;
    while (*cmd && i < MAX_ARGS - 1) {
        while (*cmd == ' ') *cmd++ = 0; /* Skip spaces and terminate prev arg */
        if (*cmd == 0) break;
        args[i++] = cmd;
        while (*cmd && *cmd != ' ') cmd++;
    }
    args[i] = 0;
}

int main() {
    char cmd[128];
    char* args[MAX_ARGS];
    
    printf("\nWelcome to Cortez-OS Shell (Userspace)\n");
    
    while (1) {
        char cwd[256];
        getcwd(cwd, 256);
        printf("%s> ", cwd);
        
        gets(cmd, 128);
        
        if (cmd[0] == 0) continue;
        
        split_command(cmd, args);
        if (!args[0]) continue;
        
        if (strcmp(args[0], "exit") == 0) {
            break;
        } else if (strcmp(args[0], "cd") == 0) {
            if (args[1]) {
                if (chdir(args[1]) != 0) {
                    printf("cd: %s: No such directory\n", args[1]);
                }
            }
        } else if (strcmp(args[0], "help") == 0) {
            printf("Available commands:\n");
            printf("  cd <dir>    - Change directory\n");
            printf("  exit        - Exit shell\n");
            printf("  help        - Show this message\n");
            printf("  ls          - List files (external binary)\n");
            printf("  cat <file>  - Show file (external binary)\n");
            printf("  mkdir <dir> - Create dir (external binary)\n");
            printf("  rm <file>   - Remove file (external binary)\n");
        } else {
            /* Try to execute binary */
            int pid = fork();
            if (pid == 0) {
                /* Child */
                char bin_path[256];
                
                /* Check if absolute/relative path */
                if (args[0][0] == '/' || (args[0][0] == '.' && args[0][1] == '/')) {
                    strcpy(bin_path, args[0]);
                } else {
                    /* Search in /bin */
                    strcpy(bin_path, "/bin/");
                    strcat(bin_path, args[0]);
                }
                
                if (exec(bin_path) < 0) {
                    printf("Command not found: %s\n", args[0]);
                    exit(1);
                }
            } else {
                /* Parent */
                int status;
                waitpid(pid, &status, 0);
            }
        }
    }
    return 0;
}
