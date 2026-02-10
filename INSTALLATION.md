# INSTALLATION

## Quick installation from binary files

### Option 1: evDash Web Flasher (recommended for users)
- Open https://www.evdash.eu/m5flash
- Use Chrome/Edge on desktop and a USB data cable.
- Select the correct board profile and click `INSTALL`.

### Option 2: Generic ESP web flasher
- https://esp.huhn.me/
- Use files from `dist/<board>/` with offsets:
  - `0x1000` bootloader
  - `0x8000` partitions
  - `0xE000` boot_app0
  - `0x10000` firmware

Partition settings example:
![image](https://github.com/nickn17/evDash/blob/master/docs/webflasher_esp_huhn_me.png)

### Option 3: Command line flashing (esptool)
Example for Core2 v1.0:

```bash
esptool.py -p /dev/ttyUSB0 -b 921600 write_flash \
  --flash_mode dio --flash_size detect --flash_freq 80m \
  0x1000 dist/m5stack-core2-v1_0/bootloader_qio_80m.bin \
  0x8000 dist/m5stack-core2-v1_0/evDash.ino.partitions.bin \
  0xE000 dist/m5stack-core2-v1_0/boot_app0.bin \
  0x10000 dist/m5stack-core2-v1_0/evDash.ino.bin
```

Older guide:
- M5STACK (Many thanks to DimZen)
- https://docs.google.com/document/d/17vJmeveNfN0exQy9wKC-5igU8zzNjsuOn1DPuPV_yJA/edit?usp=sharing

## Installation from sources
- install VS code (https://code.visualstudio.com/)
- install Platformio (https://platformio.org/install/ide?install=vscode)
- import project evDash to platformio (uncheck import libraries from Arduino, select board: m5stack core, or core2)
- copy platformio.ini.example to platformio.ini and edit

## Serial console
- Web tools by https://serial.huhn.me/
