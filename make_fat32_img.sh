#!/bin/sh

# FAT32 Image Creator
# Creates a blank disk image with a FAT32 formatted partition
# Usage: ./create_fat32_image.sh <image_path> <Size> <GB/MB>
# Example: ./create_fat32_image.sh /mnt/data/iso/SystemDisk.img 1GB

# Validate arguments
if [ $# -ne 3 ]; then
    echo "Error: Incorrect number of arguments."
    echo "Usage: $0 <ImagePath> <Size> <GB/MB>"
    echo "Example: $0 /mnt/data/iso/Image01.img 1 GB"
    exit 1
fi

IMG_PATH="$1"
SIZE_VALUE="$2"
SIZE_UNIT="$3"

# Convert to Mbytes
case $SIZE_UNIT in
    MB)
        BS_COUNT=$SIZE_VALUE
        ;;
    GB)
        BS_COUNT=$((SIZE_VALUE*1024))
        ;;
    *)
        echo "Error: Unspported unit  '$SIZE_UNIT'"
        exit 1
        ;;
esac

# Create directory if needed
IMG_DIR=$(dirname "$IMG_PATH")
if [ ! -d "$IMG_DIR" ]; then
    echo "创建目录: $IMG_DIR"
    mkdir -p "$IMG_DIR" || exit 1
fi

# Create blank image file
echo "Creating ${SIZE_VALUE}${SIZE_UNIT} image file: $IMG_PATH"
dd if=/dev/zero of="$IMG_PATH" bs=1MB count=$BS_COUNT status=progress || {
    echo "Image creation failed!"
    exit 1
}

# Create partition table
echo "Creating partition table..."
parted "$IMG_PATH" --script mklabel msdos || {
    echo "Partition table creation failed!"
    exit 1
}

# Create primary FAT32 partition (leave 1MB for boot)
echo "Creating FAT32 partition..."
parted "$IMG_PATH" --script mkpart primary fat32 1MiB 100% || {
    echo "Partition creation failed!"
    exit 1
}

# Set up loop device
echo "Configuring loop device..."
LOOP_DEV=$(losetup -P /dev/loop0 "$IMG_PATH") || {
    echo "Loop device setup failed!"
    exit 1
}

# Get partition path
PARTITION="/dev/loop0p1"

# 格式化分区为FAT32
echo "Formatting as FAT32..."
mkdosfs -n "DATA" "$PARTITION" || {
    echo "Formatting failed!"
    losetup -d "/dev/loop0"
    exit 1
}

# Clean up loop device
losetup -d "/dev/loop0"

# Success message
echo "=============================================="
echo "Image successfully created: $IMAGE_PATH"
echo "Filesystem: FAT32"
echo "Image size: $SIZE_VALUE$SIZE_UNIT"
echo "Volume label: DATA_VOLUME"
echo "=============================================="