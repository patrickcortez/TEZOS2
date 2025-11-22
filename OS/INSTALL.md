# Cortez-OS Hardware Installation Guide

## 🎯 Install Your OS on Real Hardware

This guide will install Cortez-OS as a **real, independent operating system** on your physical computer, appearing in GRUB alongside Ubuntu, Windows, or any other OS.

## ⚠️ Prerequisites

1. **Dedicated Partition** (100MB minimum, 1GB recommended)
   - Use `gparted` or `fdisk` to create an empty partition
   - Mark as **unallocated** or **free space**
   - **ALL DATA WILL BE ERASED** from this partition

2. **Backup Your Data**
   - Installation modifies GRUB bootloader
   - Backup important files before proceeding

3. **Ubuntu/Linux System**
   - GRUB2 bootloader installed
   - Root/sudo access required

## 📦 Installation Steps

### Step 1: Build the Kernel
```bash
cd /home/cortez/Documents/Projects/2D\ Engine/OS
make clean && make
```

### Step 2: Check Available Partitions
```bash
sudo fdisk -l
# or
lsblk
```

Look for an empty partition (e.g., `/dev/sda3`, `/dev/nvme0n1p3`)

### Step 3: Run the Installer
```bash
sudo ./install.sh
```

### Step 4: Follow Prompts
1. Select target partition (e.g., `/dev/sda3`)
2. Type `YES` to confirm
3. Type `INSTALL` for final confirmation
4. Wait for installation to complete

### Step 5: Reboot
```bash
sudo reboot
```

## 🚀 Booting Cortez-OS

1. **Power on** your computer
2. **GRUB menu** will appear with boot options
3. **Select "Cortez-OS"** from the menu
4. Your OS boots! 🎉

## 📝 What Gets Installed

```
/dev/sdaX (Your partition)
├── boot/
│   └── kernel.bin          # Cortez-OS kernel
└── [ext2 filesystem]

/etc/grub.d/40_custom        # GRUB entry added
```

## 🔧 Post-Installation

Once booted into Cortez-OS:
```bash
> format          # Format the filesystem
> initfs          # Initialize root filesystem
> ls /            # See your Unix directory structure
> cat /README     # Read documentation
```

## 🛠️ Troubleshooting

### "Cortez-OS doesn't appear in GRUB"
```bash
sudo update-grub
sudo reboot
```

### "Boot fails / Black screen"
- Verify partition UUID in `/etc/grub.d/40_custom`
- Check kernel copied correctly: `sudo mount /dev/sdaX /mnt && ls /mnt/boot`

### "Want to remove Cortez-OS"
```bash
# Remove GRUB entry
sudo nano /etc/grub.d/40_custom  # Delete Cortez-OS section
sudo update-grub

# Format partition (optional)
sudo mkfs.ext4 /dev/sdaX  # Replace sdaX with your partition
```

## 💡 Tips

- **Test in VM first**: Try with VirtualBox/QEMU multi-boot setup
- **Use small partition**: 100MB-500MB is enough
- **Keep Ubuntu bootable**: Don't touch other partitions
- **Backup GRUB**: Installer auto-backs up to `40_custom.backup.*`

## 🎓 What This Proves

✅ **Real OS**: Independent partition, not a VM  
✅ **Bootloader Integration**: GRUB recognizes it as legitimate OS  
✅ **Multi-boot**: Works alongside Ubuntu/Windows  
✅ **Hardware Access**: Direct hardware access, not emulated  
✅ **Production Ready**: Can be installed like Linux/Windows  

**This is a REAL operating system!** 🚀
