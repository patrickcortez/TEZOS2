default: build

build:
	mkdir -p build/isofiles/boot/grub
	nasm -f elf64 boot/header.asm -o build/header.o
	nasm -f elf64 boot/main.asm -o build/main.o
	nasm -f elf64 boot/main64.asm -o build/main64.o
	nasm -f elf64 kernel/interrupts.asm -o build/interrupts.o
	
	gcc -c kernel/main.c -o build/kernel.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/idt.c -o build/idt.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/pic.c -o build/pic.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/keyboard.c -o build/keyboard.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/pmm.c -o build/pmm.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/pit.c -o build/pit.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/shell.c -o build/shell.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/shell.c -o build/shell.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/video.c -o build/video.o -ffreestanding -mno-red-zone -m64 -I./kernel
	gcc -c kernel/serial.c -o build/serial.o -ffreestanding -mno-red-zone -m64 -I./kernel
	
	ld -n -o build/isofiles/boot/kernel.bin -T linker.ld build/header.o build/main.o build/main64.o build/interrupts.o build/kernel.o build/idt.o build/pic.o build/keyboard.o build/pmm.o build/pit.o build/shell.o build/video.o build/serial.o
	
	# Create Grub config
	echo 'set timeout=0' > build/isofiles/boot/grub/grub.cfg
	echo 'set default=0' >> build/isofiles/boot/grub/grub.cfg
	echo '' >> build/isofiles/boot/grub/grub.cfg
	echo 'insmod all_video' >> build/isofiles/boot/grub/grub.cfg
	echo 'insmod vbe' >> build/isofiles/boot/grub/grub.cfg
	echo 'insmod vga' >> build/isofiles/boot/grub/grub.cfg
	echo '' >> build/isofiles/boot/grub/grub.cfg
	echo 'set gfxmode=1024x768x32' >> build/isofiles/boot/grub/grub.cfg
	echo 'terminal_output gfxterm' >> build/isofiles/boot/grub/grub.cfg
	echo '' >> build/isofiles/boot/grub/grub.cfg
	echo 'menuentry "Cortez-OS" {' >> build/isofiles/boot/grub/grub.cfg
	echo '  multiboot2 /boot/kernel.bin' >> build/isofiles/boot/grub/grub.cfg
	echo '  boot' >> build/isofiles/boot/grub/grub.cfg
	echo '}' >> build/isofiles/boot/grub/grub.cfg
	
	grub-mkrescue -o cortez-os.iso build/isofiles

clean:
	rm -rf build cortez-os.iso
