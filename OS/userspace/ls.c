#include "libc.h"

int main() {
    char cwd[256];
    getcwd(cwd, 256);
    
    int fd = open(cwd, O_RDONLY);
    if (fd < 0) {
        printf("ls: cannot open directory\n");
        return 1;
    }
    
    dir_entry_t entry;
    while (readdir(fd, &entry) == 0) {
        printf("%s", entry.name);
        if (entry.is_directory) printf("/");
        printf("\n");
    }
    
    close(fd);
    return 0;
}
