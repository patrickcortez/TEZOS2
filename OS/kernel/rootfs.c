#include "rootfs.h"
#include "fs.h"
#include "video.h"

void rootfs_init() {
    print_str("Initializing root filesystem...\n");
    
    /* Create standard Unix directory structure */
    fs_mkdir("/bin");      /* User binaries */
    fs_mkdir("/sbin");     /* System binaries */
    fs_mkdir("/etc");      /* Configuration files */
    fs_mkdir("/home");     /* User home directories */
    fs_mkdir("/tmp");      /* Temporary files */
    fs_mkdir("/var");      /* Variable data */
    fs_mkdir("/dev");      /* Device files */
    fs_mkdir("/proc");     /* Process information */
    fs_mkdir("/usr");
    fs_mkdir("/usr/bin");
    fs_mkdir("/usr/lib");
    fs_mkdir("/usr/share");
    
    /* Create system files */
    const char* motd = "Welcome to Cortez-OS v1.0\n"
                      "A robust, ExFAT-powered operating system.\n"
                      "Type 'help' for commands.\n";
    fs_create("/etc/motd");
    fs_write_file("/etc/motd", motd, 0, 90);
    
    const char* hostname = "cortez-os\n";
    fs_create("/etc/hostname");
    fs_write_file("/etc/hostname", hostname, 0, 10);
    
    const char* version = "Cortez-OS 1.0.0\nKernel: 1.0.0-pre\nArch: x86_64\nFS: ExFAT\n";
    fs_create("/etc/version");
    fs_write_file("/etc/version", version, 0, 60);
    
    /* Create README in root */
    const char* readme = "CORTEZ-OS ROOT FILESYSTEM\n\n"
                        "This filesystem is formatted with ExFAT.\n"
                        "It supports:\n"
                        "- Long filenames (up to 15 chars for now)\n"
                        "- Large files\n"
                        "- Deep directory nesting\n"
                        "- Full CRUD operations\n\n"
                        "Try creating directories and files!\n";
    fs_create("/README");
    fs_write_file("/README", readme, 0, 200);
    
    /* Create userspace binaries */
    /* We need to read them from the build directory? No, we are in the kernel.
       We need to have them linked in or loaded as modules.
       
       Wait, the kernel cannot read files from the host build directory.
       The only way to get them into the disk image is:
       1. Load them as Multiboot modules (like kernel.bin).
       2. Or link them into the kernel binary as data arrays (incbin).
       
       Let's use the "incbin" approach via assembly or just assume they are modules.
       Actually, loading multiple modules is cleaner.
       
       BUT, the user wants "real binaries".
       
       Let's modify the Makefile to add them as modules in GRUB config!
       Then the kernel can find them in memory and write them to disk.
       
       Let's update rootfs.c to look for modules named "shell", "ls", "cat".
    */
    
    extern u64 find_module(const char* name, u64* size);
    
    u64 shell_addr, shell_size;
    if ((shell_addr = find_module("shell", &shell_size))) {
        fs_create("/bin/shell");
        fs_write_file("/bin/shell", (void*)shell_addr, 0, shell_size);
        print_str("Installed /bin/shell\n");
    }
    
    u64 ls_addr, ls_size;
    if ((ls_addr = find_module("ls", &ls_size))) {
        fs_create("/bin/ls");
        fs_write_file("/bin/ls", (void*)ls_addr, 0, ls_size);
        print_str("Installed /bin/ls\n");
    }
    
    u64 cat_addr, cat_size;
    if ((cat_addr = find_module("cat", &cat_size))) {
        fs_create("/bin/cat");
        fs_write_file("/bin/cat", (void*)cat_addr, 0, cat_size);
        print_str("Installed /bin/cat\n");
    }
    
    /* Create sample user home */
    fs_mkdir("/home/cortez");
    const char* user_profile = "# User Profile\n"
                              "USER=cortez\n"
                              "HOME=/home/cortez\n"
                              "SHELL=/bin/shell\n"
                              "PATH=/bin:/usr/bin\n";
    fs_create("/home/cortez/.profile");
    fs_write_file("/home/cortez/.profile", user_profile, 0, 80);
    
    fs_mkdir("/home/cortez/projects");
    fs_create("/home/cortez/projects/todo.txt");
    fs_write_file("/home/cortez/projects/todo.txt", "- Fix bugs\n- Implement GUI\n- Sleep\n", 0, 35);
    
    /* Create fstab */
    const char* fstab = "# Filesystem Table\n"
                       "/dev/hda1  /  exfat  defaults  0  1\n"
                       "proc       /proc proc defaults 0 0\n";
    fs_create("/etc/fstab");
    fs_write_file("/etc/fstab", fstab, 0, 80);
    
    /* Create passwd file */
    const char* passwd = "root:x:0:0:root:/root:/bin/shell\n"
                        "cortez:x:1000:1000:Cortez:/home/cortez:/bin/shell\n";
    fs_create("/etc/passwd");
    fs_write_file("/etc/passwd", passwd, 0, 90);
    
    /* Create group file */
    const char* group = "root:x:0:\n"
                       "users:x:100:\n"
                       "cortez:x:1000:\n";
    fs_create("/etc/group");
    fs_write_file("/etc/group", group, 0, 40);
    
    /* Create /boot and kernel files */
    fs_mkdir("/boot");
    fs_mkdir("/boot/grub");
    
    const char* grub_cfg = "set timeout=0\nmenuentry \"Cortez-OS\" {\n  multiboot2 /boot/kernel.bin\n  boot\n}\n";
    fs_create("/boot/grub/grub.cfg");
    fs_write_file("/boot/grub/grub.cfg", grub_cfg, 0, 80);
    
    /* Write kernel binary */
    fs_create("/boot/kernel.bin");
    
    extern u64 kernel_module_addr;
    extern u64 kernel_module_size;
    
    if (kernel_module_addr != 0 && kernel_module_size > 0) {
        print_str("Writing kernel binary from memory...\n");
        fs_write_file("/boot/kernel.bin", (void*)kernel_module_addr, 0, kernel_module_size);
    } else {
        print_str("WARNING: Kernel module not found. Using placeholder.\n");
        const char* kernel_sig = "\x7F\x45\x4C\x46\x02\x01\x01\x00 (Cortez-OS Kernel Image)";
        fs_write_file("/boot/kernel.bin", kernel_sig, 0, 40);
    }
    
    fs_mkdir("/System");
    fs_mkdir("/System/Drivers");
    fs_create("/System/Drivers/ata.drv");
    fs_create("/System/Drivers/vga.drv");
    fs_create("/System/Drivers/ps2.drv");
    
    const char* hw_info = "Cortez-OS Hardware Requirements:\n"
                         "- Architecture: x86_64 (64-bit)\n"
                         "- Firmware: BIOS or UEFI in CSM (Legacy) Mode\n"
                         "- Storage: IDE/ATA Controller (Legacy Mode)\n"
                         "  * Note: AHCI/RAID modes not yet supported\n"
                         "- Input: PS/2 Keyboard (or USB with BIOS emulation)\n";
    fs_create("/HARDWARE.TXT");
    fs_write_file("/HARDWARE.TXT", hw_info, 0, 200);
    
    print_str("Root filesystem initialized with ExFAT structure.\n");
}
