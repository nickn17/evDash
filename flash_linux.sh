#!/bin/bash

# Setup
# 1. Ensure you have Python 2.7 or 3.4+ installed
# 2. `pip install esptool`
# 3. `sudo usermod -a -G dialout <USERNAME>`   // log out and log in is required

# Select distribution:
# TTGO T4
DIST=./dist/ttgo_t4_v13

# or m5stack
DIST=./dist/m5stack_core1

# Set USB port
PORT=/dev/ttyUSB0

esptool.py -p $PORT -b 921600 write_flash  \
  --flash_mode dio                         \
  --flash_size detect                      \
  --flash_freq 80m                         \
  0xe000 $DIST/boot_app0.bin               \
  0x1000 $DIST/bootloader_qio_80m.bin      \
  0x10000 $DIST/evDash.ino.bin             \
  0x8000 $DIST/evDash.ino.partitions.bin
