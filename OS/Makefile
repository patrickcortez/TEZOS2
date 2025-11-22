default: build

build:
	mkdir -p build/isofiles/boot/grub
	nasm -f elf64 boot/header.asm -o build/header.o
	nasm -f elf64 boot/main.asm -o build/main.o
	nasm -f elf64 boot/main64.asm -o build/main64.o
	nasm -f elf64 kernel/interrupts.asm -o build/interrupts.o
	nasm -f elf64 kernel/context.asm -o build/context.o
	nasm -f elf64 kernel/gdt.asm -o build/gdt_asm.o
	
	gcc -c kernel/main.c -o build/kernel.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/gdt.c -o build/gdt.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/idt.c -o build/idt.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/exception.c -o build/exception.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/syscall.c -o build/syscall.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/string.c -o build/string.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/vmm.c -o build/vmm.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/task.c -o build/task.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/elf.c -o build/elf.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/signal.c -o build/signal.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/pic.c -o build/pic.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/keyboard.c -o build/keyboard.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/pmm.c -o build/pmm.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/pit.c -o build/pit.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/shell.c -o build/shell.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/shell.c -o build/shell.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/video.c -o build/video.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/serial.c -o build/serial.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/heap.c -o build/heap.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/ata.c -o build/ata.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/fs.c -o build/fs.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/mbr.c -o build/mbr.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/rootfs.c -o build/rootfs.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/installer.c -o build/installer.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/editor.c -o build/editor.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/reboot.c -o build/reboot.o -ffreestanding -mno-red-zone -m64 -I./kernel
	
	# Userspace Build
	mkdir -p build/userspace
	nasm -f elf64 userspace/crt0.asm -o build/userspace/crt0.o
	gcc -c userspace/libc.c -o build/userspace/libc.o -ffreestanding -mno-red-zone -m64 -I./userspace
	
	# Shell
	gcc -c userspace/shell.c -o build/userspace/shell.o -ffreestanding -mno-red-zone -m64 -I./userspace
	ld -n -Ttext 0x400000 -o build/userspace/shell build/userspace/crt0.o build/userspace/shell.o build/userspace/libc.o
	
	# LS
	gcc -c userspace/ls.c -o build/userspace/ls.o -ffreestanding -mno-red-zone -m64 -I./userspace
	ld -n -Ttext 0x400000 -o build/userspace/ls build/userspace/crt0.o build/userspace/ls.o build/userspace/libc.o
	
	# CAT
	gcc -c userspace/cat.c -o build/userspace/cat.o -ffreestanding -mno-red-zone -m64 -I./userspace
	ld -n -Ttext 0x400000 -o build/userspace/cat build/userspace/crt0.o build/userspace/cat.o build/userspace/libc.o

	ld -n -T linker.ld -o build/kernel.bin build/header.o build/main.o build/main64.o \
		build/interrupts.o build/context.o build/gdt_asm.o build/kernel.o build/gdt.o \
		build/idt.o build/exception.o build/syscall.o build/string.o build/vmm.o \
		build/task.o build/elf.o build/signal.o build/pic.o build/keyboard.o build/pmm.o \
		build/pit.o build/shell.o build/video.o build/serial.o build/heap.o build/ata.o \
		build/fs.o build/mbr.o build/rootfs.o build/installer.o build/editor.o build/reboot.o
	
	# Create Disk Image with MBR Partition (10MB)
	dd if=/dev/zero of=disk.img bs=1M count=10 2>/dev/null
	# Write MBR with single partition
	printf '\x00%.0s' {1..446} > /tmp/mbr.bin
	# Partition 1: Bootable, Type 0x83 (Linux), LBA 2048, Size 18432 sectors (2048-20479)
	printf '\x80\x00\x01\x00\x83\xFE\xFF\xFF\x00\x08\x00\x00\x00\x48\x00\x00' >> /tmp/mbr.bin
	printf '\x00%.0s' {1..48} >> /tmp/mbr.bin
	printf '\x55\xAA' >> /tmp/mbr.bin
	dd if=/tmp/mbr.bin of=disk.img bs=512 count=1 conv=notrunc 2>/dev/null
	rm /tmp/mbr.bin
	
	# Create Grub config
	echo 'set timeout=0' > build/isofiles/boot/grub/grub.cfg
	echo 'set default=0' >> build/isofiles/boot/grub/grub.cfg
	echo '' >> build/isofiles/boot/grub/grub.cfg
	echo 'insmod all_video' >> build/isofiles/boot/grub/grub.cfg
	echo 'insmod vbe' >> build/isofiles/boot/grub/grub.cfg
	echo 'insmod vga' >> build/isofiles/boot/grub/grub.cfg
	echo '' >> build/isofiles/boot/grub/grub.cfg
	echo 'set gfxmode=1024x768x32' >> build/isofiles/boot/grub/grub.cfg
	echo 'set gfxpayload=1024x768x32' >> build/isofiles/boot/grub/grub.cfg
	echo 'terminal_output gfxterm' >> build/isofiles/boot/grub/grub.cfg
	echo '' >> build/isofiles/boot/grub/grub.cfg
	echo 'menuentry "Cortez-OS" {' >> build/isofiles/boot/grub/grub.cfg
	echo '  multiboot2 /boot/kernel.bin' >> build/isofiles/boot/grub/grub.cfg
	echo '  module2 /boot/kernel.bin cmdline=kernel' >> build/isofiles/boot/grub/grub.cfg
	echo '  module2 /boot/shell cmdline=shell' >> build/isofiles/boot/grub/grub.cfg
	echo '  module2 /boot/ls cmdline=ls' >> build/isofiles/boot/grub/grub.cfg
	echo '  module2 /boot/cat cmdline=cat' >> build/isofiles/boot/grub/grub.cfg
	echo '  boot' >> build/isofiles/boot/grub/grub.cfg
	echo '}' >> build/isofiles/boot/grub/grub.cfg
	
	# Copy userspace binaries to ISO boot folder so GRUB can find them
	cp build/userspace/shell build/isofiles/boot/shell
	cp build/userspace/ls build/isofiles/boot/ls
	cp build/userspace/cat build/isofiles/boot/cat
	
	grub-mkrescue -o cortez-os.iso build/isofiles

clean:
	rm -rf build cortez-os.iso
