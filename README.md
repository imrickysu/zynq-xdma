# Use XDMA in PetaLinux

```
# Create PetaLinux Project
petalinux-create -t project -n petalinux-2014.4

# Import Vivado Project Settings
# Please copy Vivado exported hdf file to current folder
cd petalinux-2014.4
petalinux-config --get-hw-description=./

# Create Components
petalinux-create -t modules -n xdma-dev --enable
petalinux-create -t libs -n xdma-lib --enable
petalinux-create -t apps -n xdma-app --enable

# Copy source files
cp ../dev/*.c ../dev/*.h components/modules/xdma-dev/
cp ../lib/*.c ../lib/*.h components/libs/xdma-lib/
cp ../demo/xdma-app.c components/apps/xdma-app/

# Update Makefile. Use the ones in this repo

# Build petalinux
petalinux-build

# Package
petalinux-package --boot --fsbl images/linux/zynq_fsbl.elf --fpga subsystems/linux/hw-description/design_1_wrapper.bit --u-boot

# Test
# Copy BOOT.BIN and image.ub to SD card
```

## Test on Target ##
```
# Login as root

# Find the xdma related files
find / -name "xdma*"

# Insert xdma kernel module
insmod /lib/modules/3.17.0-xilinx/extra/xdma.ko

# Execute the demo
/bin/xdma-app

```
