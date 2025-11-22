#!/bin/bash
# Cortez-OS Real Hardware Installer
# Installs Cortez-OS to dedicated partition with GRUB integration

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   CORTEZ-OS HARDWARE INSTALLER         ║${NC}"
echo -e "${BLUE}║   Real OS Installation System          ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════╝${NC}"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}ERROR: This installer must be run as root${NC}"
    echo "Please run: sudo ./install.sh"
    exit 1
fi

# Check prerequisites
echo -e "${YELLOW}[1/8] Checking prerequisites...${NC}"
command -v grub-install >/dev/null 2>&1 || { echo -e "${RED}ERROR: grub-install not found${NC}"; exit 1; }
command -v update-grub >/dev/null 2>&1 || { echo -e "${RED}ERROR: update-grub not found${NC}"; exit 1; }
echo -e "${GREEN}✓ All prerequisites met${NC}"

# Build kernel if needed
echo -e "${YELLOW}[2/8] Building Cortez-OS kernel...${NC}"
if [ ! -f "build/isofiles/boot/kernel.bin" ]; then
    make clean && make
fi
echo -e "${GREEN}✓ Kernel ready${NC}"

# Detect partitions
echo -e "${YELLOW}[3/8] Detecting available partitions...${NC}"
echo ""
lsblk -o NAME,SIZE,TYPE,FSTYPE,MOUNTPOINT,LABEL
echo ""

# Show warning
echo -e "${RED}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${RED}║                    ⚠️  WARNING  ⚠️                          ║${NC}"
echo -e "${RED}║                                                            ║${NC}"
echo -e "${RED}║  This will FORMAT a partition and install Cortez-OS!      ║${NC}"
echo -e "${RED}║  ALL DATA on selected partition will be DESTROYED!        ║${NC}"
echo -e "${RED}║  Your GRUB bootloader will be modified!                   ║${NC}"
echo -e "${RED}║                                                            ║${NC}"
echo -e "${RED}║  Recommended: Use empty partition (100MB minimum)         ║${NC}"
echo -e "${RED}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Partition selection
read -p "Enter target partition (e.g., /dev/sda3 or /dev/nvme0n1p3): " PARTITION

if [ ! -b "$PARTITION" ]; then
    echo -e "${RED}ERROR: $PARTITION is not a valid block device${NC}"
    exit 1
fi

# Show partition info
echo ""
echo -e "${BLUE}Selected partition:${NC}"
lsblk -o NAME,SIZE,TYPE,FSTYPE,MOUNTPOINT,LABEL "$PARTITION"
echo ""

# Confirmation 1
read -p "$(echo -e ${YELLOW}Type 'YES' to FORMAT $PARTITION and install Cortez-OS: ${NC})" CONFIRM1
if [ "$CONFIRM1" != "YES" ]; then
    echo "Installation cancelled."
    exit 0
fi

# Confirmation 2
read -p "$(echo -e ${RED}Final confirmation - ALL DATA will be LOST. Type 'INSTALL': ${NC})" CONFIRM2
if [ "$CONFIRM2" != "INSTALL" ]; then
    echo "Installation cancelled."
    exit 0
fi

echo ""
echo -e "${GREEN}Starting installation...${NC}"
echo ""

# Unmount if mounted
echo -e "${YELLOW}[4/8] Preparing partition...${NC}"
umount "$PARTITION" 2>/dev/null || true

# Format partition as ext2
echo -e "${YELLOW}[5/8] Formatting $PARTITION as ext2...${NC}"
mkfs.ext2 -F -L "CORTEZ-OS" "$PARTITION"
echo -e "${GREEN}✓ Partition formatted${NC}"

# Create mount point and mount
MOUNT_POINT="/mnt/cortez-install"
mkdir -p "$MOUNT_POINT"
mount "$PARTITION" "$MOUNT_POINT"

# Create directory structure
echo -e "${YELLOW}[6/8] Installing Cortez-OS files...${NC}"
mkdir -p "$MOUNT_POINT/boot"
cp build/isofiles/boot/kernel.bin "$MOUNT_POINT/boot/"
echo -e "${GREEN}✓ Kernel installed to $PARTITION${NC}"

# Get partition UUID
PART_UUID=$(blkid -s UUID -o value "$PARTITION")
echo "Partition UUID: $PART_UUID"

# Unmount
umount "$MOUNT_POINT"

# Create GRUB entry
echo -e "${YELLOW}[7/8] Configuring GRUB bootloader...${NC}"

# Backup existing GRUB config
cp /etc/grub.d/40_custom /etc/grub.d/40_custom.backup.$(date +%s)

# Determine partition format (e.g., sda3 -> hd0,3 or nvme0n1p3 -> hd0,gpt3)
PARTITION_NAME=$(basename "$PARTITION")
DISK_NAME=$(echo "$PARTITION_NAME" | sed 's/[0-9]*$//')
PART_NUM=$(echo "$PARTITION_NAME" | grep -o '[0-9]*$')

# Add GRUB entry
cat >> /etc/grub.d/40_custom << EOF

# Cortez-OS Entry (Added by installer)
menuentry 'Cortez-OS' {
    insmod part_gpt
    insmod part_msdos
    insmod ext2
    search --no-floppy --fs-uuid --set=root $PART_UUID
    multiboot2 /boot/kernel.bin
    boot
}
EOF

echo -e "${GREEN}✓ GRUB entry created${NC}"

# Update GRUB
echo -e "${YELLOW}[8/8] Updating GRUB configuration...${NC}"
update-grub
echo -e "${GREEN}✓ GRUB updated${NC}"

# Success message
echo ""
echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║              ✓ INSTALLATION SUCCESSFUL ✓                  ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${BLUE}Cortez-OS has been installed to: ${NC}$PARTITION"
echo -e "${BLUE}Partition UUID: ${NC}$PART_UUID"
echo -e "${BLUE}Mount point: ${NC}/boot (when booted)"
echo ""
echo -e "${YELLOW}Next steps:${NC}"
echo "  1. Reboot your computer"
echo "  2. At GRUB menu, select 'Cortez-OS'"
echo "  3. Your OS will boot!"
echo ""
echo -e "${BLUE}Partition information:${NC}"
lsblk -o NAME,SIZE,TYPE,FSTYPE,MOUNTPOINT,LABEL "$PARTITION"
echo ""
echo -e "${GREEN}Installation complete. Reboot when ready!${NC}"
echo ""

# Cleanup
rmdir "$MOUNT_POINT" 2>/dev/null || true
