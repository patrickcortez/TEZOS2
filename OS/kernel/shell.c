#include "types.h"
#include "video.h"

#define CMD_BUFFER_SIZE 128
static char cmd_buffer[CMD_BUFFER_SIZE];
static int cmd_index = 0;

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void execute_command() {
    print_str("\n");
    if (cmd_index == 0) {
        print_str("> ");
        return;
    }
    
    cmd_buffer[cmd_index] = 0; /* Null terminate */
    
    if (strcmp(cmd_buffer, "help") == 0) {
        print_str("Available commands:\n");
        print_str("  help    - Show this message\n");
        print_str("  clear   - Clear the screen\n");
        print_str("  reboot  - Reboot the system\n");
    } else if (strcmp(cmd_buffer, "clear") == 0) {
        clear_screen();
    } else if (strcmp(cmd_buffer, "reboot") == 0) {
        print_str("Rebooting...\n");
        /* Triple fault to reboot */
        /* __asm__ volatile("lidt 0"); */ /* This would crash, but let's just print for now */
    } else {
        print_str("Unknown command: ");
        print_str(cmd_buffer);
        print_str("\n");
    }
    
    cmd_index = 0;
    print_str("> ");
}

void shell_handle_key(char c) {
    if (c == '\n') {
        execute_command();
    } else if (c == '\b') {
        if (cmd_index > 0) {
            cmd_index--;
            video_backspace();
        }
    } else {
        if (cmd_index < CMD_BUFFER_SIZE - 1) {
            cmd_buffer[cmd_index++] = c;
            print_char(c);
        }
    }
}
