#!/bin/bash

set -e  # Exit immediately if a command exits with a non-zero status.

# Default paths
EFI_DISK="./efi.disk"
EFI_MOUNT_DIR="/tmp/efi"
WOLFCRYPT_EFI="wolfcrypt.efi"
STARTUP_NSH="startup.nsh"
QEMU_BIOS="/usr/share/OVMF/OVMF_CODE.fd"
WOLFSSL_DIR="wolfssl"

# Self-updating script paths
SCRIPT_FILE=$(realpath "$0")

update_script_paths() {
    echo "Updating paths in the script..."
    for var in QEMU_BIOS; do
        current_path=$(eval echo \$$var)
        echo "Current path for $var: $current_path"
        read -p "Do you want to update this path? (y/N): " confirm
        if [[ "$confirm" =~ ^[Yy]$ ]]; then
            read -p "Enter new path for $var: " new_path
            sed -i "s|^$var=\".*\"|$var=\"$new_path\"|" "$SCRIPT_FILE"
            eval "$var=\"$new_path\""
        fi
    done
}

prompt_for_path() {
    local var_name="$1"
    local current_path="$2"
    local prompt_msg="$3"

    while [ ! -e "$current_path" ]; do
        echo "$prompt_msg"
        read -p "Enter a valid path: " new_path
        sed -i "s|^$var_name=\".*\"|$var_name=\"$new_path\"|" "$SCRIPT_FILE"
        eval "$var_name=\"$new_path\""
        current_path="$new_path"
    done
}

# Function to create the EFI disk
create_efi_disk() {
    echo "Creating EFI disk..."
    dd if=/dev/zero of="$EFI_DISK" bs=1024M count=1
    sudo mkfs.vfat "$EFI_DISK"
}

# Function to copy files to the EFI disk
populate_efi_disk() {
    echo "Populating EFI disk..."
    mkdir -p "$EFI_MOUNT_DIR"
    sudo mount "$EFI_DISK" "$EFI_MOUNT_DIR" -o loop

    if [ ! -f "$WOLFCRYPT_EFI" ]; then
        echo "Error: $WOLFCRYPT_EFI not found."
        exit 1
    fi

    if [ ! -f "$STARTUP_NSH" ]; then
        echo "Error: $STARTUP_NSH not found."
        exit 1
    fi
    sudo cp -rf --dereference wolfssl/* "$EFI_MOUNT_DIR"
    sudo cp "$WOLFCRYPT_EFI" "$EFI_MOUNT_DIR"
    sudo cp "$STARTUP_NSH" "$EFI_MOUNT_DIR"
    sudo umount "$EFI_MOUNT_DIR"
    rmdir "$EFI_MOUNT_DIR"
}

# Function to run QEMU
run_qemu() {
    echo "Running QEMU..."
    qemu-system-x86_64 \
        -bios "$QEMU_BIOS" \
        -display none \
        -serial stdio \
        -net none \
        -m 512M \
        -drive file="$EFI_DISK",index=0,media=disk,format=raw \
        -object rng-random,filename=/dev/urandom,id=rng0 \
        -device virtio-rng-pci,rng=rng0
}

# Main script
if [[ "$1" == "-u" ]]; then
    update_script_paths
    exit 0
fi

prompt_for_path QEMU_BIOS "$QEMU_BIOS" "The path to the QEMU BIOS is invalid or not set."

if [ ! -f "$EFI_DISK" ]; then
    create_efi_disk
fi

populate_efi_disk
run_qemu

