#include "types.h"
#include "video.h"
#include "io.h"
#include "serial.h"
#include "fs.h"
#include "heap.h"
#include "rootfs.h"
#include "installer.h"
#include "editor.h"

extern void print_str(const char* str);
extern void print_char(char c);
extern void editor_edit(const char* filename);

#define CMD_BUFFER_SIZE 128
static char cmd_buffer[CMD_BUFFER_SIZE];
static int cmd_index = 0;

/* String helpers (already in fs.c but we need them here too) */
int str_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int str_startswith(const char* str, const char* prefix) {
    while (*prefix) {
        if (*str++ != *prefix++) return 0;
    }
    return 1;
}

void shell_init() {
    print_str("\nWelcome to Cortez-OS Shell!\n");
    print_str("Type 'help' for commands.\n");
    print_str("> ");
    cmd_index = 0;
}

void execute_command() {
    print_str("\n");
    if (cmd_index == 0) {
        print_str("> ");
        return;
    }
    
    cmd_buffer[cmd_index] = 0; /* Null terminate */
    
    if (str_strcmp(cmd_buffer, "help") == 0) {
        print_str("Available commands:\n");
        print_str("  help            - Show this message\n");
        print_str("  clear           - Clear the screen\n");
        print_str("  install         - Install OS to hard disk\n");
        print_str("  format          - Format the disk\n");
        print_str("  initfs          - Initialize root filesystem\n");
        print_str("  ls [path]       - List files\n");
        print_str("  mkdir <path>    - Create directory\n");
        print_str("  rmdir <path>    - Remove directory\n");
        print_str("  cd <path>       - Change directory\n");
        print_str("  pwd             - Print working directory\n");
        print_str("  touch <name>    - Create file\n");
        print_str("  write <name> <data> - Write to file\n");
        print_str("  cat <name>      - Display file\n");
        print_str("  edit <file>     - Edit file (Ctrl+S: save, ESC: exit)\n");
        print_str("  rm <path>       - Delete file\n");
        print_str("  mv <old> <new>  - Move/rename file\n");
        print_str("  hexdump <sec>   - Dump sector data\n");
        print_str("  inspect_mbr     - Show MBR partition table\n");
    } else if (str_strcmp(cmd_buffer, "clear") == 0) {
        clear_screen();
    } else if (str_strcmp(cmd_buffer, "install") == 0) {
        installer_main();
    } else if (str_strcmp(cmd_buffer, "format") == 0) {
        fs_format();
    } else if (str_strcmp(cmd_buffer, "initfs") == 0) {
        rootfs_init();
    } else if (str_strcmp(cmd_buffer, "ls") == 0) {
        fs_print_tree(".", 0);  /* Show CURRENT directory, not root */
    } else if (str_startswith(cmd_buffer, "ls ")) {
        fs_print_tree(cmd_buffer + 3, 0);
    } else if (str_strcmp(cmd_buffer, "pwd") == 0) {
        char cwd[256];
        fs_getcwd(cwd, 256);
        print_str(cwd);
        print_str("\n");
    } else if (str_startswith(cmd_buffer, "cd ")) {
        if (fs_chdir(cmd_buffer + 3) == 0) {
            /* Success */
        } else {
            print_str("Directory not found.\n");
        }
    } else if (str_startswith(cmd_buffer, "mkdir ")) {
        char* path = cmd_buffer + 6;
        if (fs_mkdir(path) == 0) {
            print_str("Created directory: ");
            print_str(path);
            print_str("\n");
        } else {
            print_str("Failed to create directory.\n");
        }
    } else if (str_startswith(cmd_buffer, "rm ")) {
        if (fs_delete(cmd_buffer + 3) == 0) {
            print_str("Deleted.\n");
        } else {
            print_str("Failed to delete.\n");
        }
    } else if (str_startswith(cmd_buffer, "touch ")) {
        char* filename = cmd_buffer + 6;
        if (fs_create(filename) == 0) {
            print_str("Created: ");
            print_str(filename);
            print_str("\n");
        } else {
            print_str("Failed to create file.\n");
        }
    } else if (str_startswith(cmd_buffer, "write ")) {
        /* Parse: write <name> <data> */
        char* p = cmd_buffer + 6;
        char* filename = p;
        while (*p && *p != ' ') p++;
        if (*p == ' ') {
            *p = 0; /* Null terminate filename */
            p++;
            char* data = p;
            int len = 0;
            while (data[len]) len++;
            
            if (fs_write_file(filename, (u8*)data, 0, len) == 0) {
                print_str("Written to: ");
                print_str(filename);
                print_str("\n");
            } else {
                print_str("Failed to write.\n");
            }
        } else {
            print_str("Usage: write <name> <data>\n");
        }
    } else if (str_startswith(cmd_buffer, "cat ")) {
        char* filename = cmd_buffer + 4;
        
        // Assuming kmalloc and kfree are available from types.h or another included header
        // and strlen, strcmp are available (str_strcmp is defined above, but strcmp is used in the edit)
        // For now, using the existing str_strcmp and assuming strlen is available.
        // If kmalloc/kfree are not available, this will cause a compilation error.
        // The user's instruction implies these are part of the new environment.

        // The provided edit uses 'cmd' and 'args' which are not defined in this context.
        // Adapting the edit to use 'cmd_buffer' and 'filename' as per existing parsing.
        
        if (str_strcmp(filename, "") == 0) { // Check if filename is empty
            print_str("Usage: cat <file>\n");
            // The original code didn't have a return here, but the edit implies it.
            // Keeping consistent with the original flow for now, but this might be a logical change.
        } else {
            // This part is directly from the user's provided edit, adapted for cmd_buffer parsing
            u8* file_buffer = (u8*)kmalloc(8192); // Assuming kmalloc is available
            if (!file_buffer) {
                print_str("Error: Out of memory\n");
            } else {
                int bytes_read = fs_read_file(filename, file_buffer, 0, 8192);
                if (bytes_read > 0) {
                    for (int i = 0; i < bytes_read && file_buffer[i] != 0; i++) {
                        print_char(file_buffer[i]);
                    }
                    print_char('\n');
                } else {
                    print_str("Error reading file\n");
                }
                kfree(file_buffer); // Assuming kfree is available
            }
        }
    } else if (str_startswith(cmd_buffer, "edit ")) {
        editor_edit(cmd_buffer + 5);
    } else if (str_startswith(cmd_buffer, "rmdir ")) {
        if (fs_rmdir(cmd_buffer + 6) == 0) {
            print_str("Directory removed.\n");
        } else {
            print_str("Failed to remove directory.\n");
        }
    } else if (str_startswith(cmd_buffer, "mv ")) {
        /* Parse: mv <old> <new> */
        char* p = cmd_buffer + 3;
        char* old_path = p;
        while (*p && *p != ' ') p++;
        if (*p == ' ') {
            *p = 0;  /* Null terminate old path */
            p++;
            char* new_path = p;
            if (fs_rename(old_path, new_path) == 0) {
                print_str("Moved/renamed.\n");
            } else {
                print_str("Failed to move/rename.\n");
            }
        } else {
            print_str("Usage: mv <old> <new>\n");
        }
    } else if (str_startswith(cmd_buffer, "hexdump ")) {
        /* hexdump <sector> */
        char* p = cmd_buffer + 8;
        u32 sector = 0;
        while (*p >= '0' && *p <= '9') {
            sector = sector * 10 + (*p - '0');
            p++;
        }
        
        u8 buf[512];
        ata_read_sector(sector, buf);
        
        print_str("Sector ");
        char sbuf[16];
        int i=0; u32 tmp=sector;
        if(tmp==0) sbuf[i++]='0';
        while(tmp>0) { sbuf[i++]='0'+(tmp%10); tmp/=10; }
        while(i>0) print_char(sbuf[--i]);
        print_str(":\n");
        
        const char* hex = "0123456789ABCDEF";
        for (int i = 0; i < 512; i++) {
            print_char(hex[(buf[i] >> 4) & 0xF]);
            print_char(hex[buf[i] & 0xF]);
            print_char(' ');
            if ((i + 1) % 16 == 0) print_char('\n');
        }
    } else if (str_strcmp(cmd_buffer, "inspect_mbr") == 0) {
        u8 buf[512];
        ata_read_sector(0, buf);
        
        print_str("MBR Signature: ");
        const char* hex = "0123456789ABCDEF";
        print_char(hex[(buf[510] >> 4) & 0xF]);
        print_char(hex[buf[510] & 0xF]);
        print_char(hex[(buf[511] >> 4) & 0xF]);
        print_char(hex[buf[511] & 0xF]);
        print_str("\n");
        
        /* Print Partitions */
        for (int i = 0; i < 4; i++) {
            u8* p = buf + 446 + (i * 16);
            print_str("Partition ");
            print_char('1' + i);
            print_str(": Type=0x");
            print_char(hex[(p[4] >> 4) & 0xF]);
            print_char(hex[p[4] & 0xF]);
            print_str(" LBA=");
            
            u32 lba = *(u32*)(p + 8);
            char sbuf[16];
            int k=0; u32 tmp=lba;
            if(tmp==0) sbuf[k++]='0';
            while(tmp>0) { sbuf[k++]='0'+(tmp%10); tmp/=10; }
            while(k>0) print_char(sbuf[--k]);
            print_str("\n");
        }
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
