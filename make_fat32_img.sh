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
    echo "Creating Folder: $IMG_DIR"
    mkdir -p "$IMG_DIR" || exit 1
fi

# Create blank image file
echo "Creating ${SIZE_VALUE}${SIZE_UNIT} image file: $IMG_PATH"
dd if=/dev/zero of="$IMG_PATH" bs=1MB count=$BS_COUNT status=noxfer|| {
    echo "Image creation failed!"
    exit 1
}
mkfs.vfat ${IMG_PATH} -n DATA_VOLUME

# Success message
echo "=============================================="
echo "Image successfully created: $IMAGE_PATH"
echo "Filesystem: FAT32"
echo "Image size: $SIZE_VALUE$SIZE_UNIT"
echo "Volume label: DATA_VOLUME"
echo "=============================================="
