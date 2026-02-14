# INSTALLATION

## Quick installation from binary files

### Option 1: evDash Web Flasher (recommended for users)
- Open https://www.evdash.eu/m5flash
- Use Chrome/Edge on desktop and a USB data cable.
- Select the correct board profile and click `INSTALL`.
- Note: `M5Stack CoreS3 SE (K128-SE)` uses the same profile/firmware as `CoreS3`.

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

## Build from source (PlatformIO)

1. Install VS Code + PlatformIO extension.
2. Clone this repository.
3. Copy `platformio.ini.example` to `platformio.ini`.
4. Set your serial `upload_port` / `monitor_port`.
5. Build one target:

```bash
~/.platformio/penv/bin/pio run -e m5stack-cores3   # CoreS3 and CoreS3 SE (K128-SE)
~/.platformio/penv/bin/pio run -e m5stack-core2-v1_0
~/.platformio/penv/bin/pio run -e m5stack-core2-v1_1
```

6. Upload:

```bash
~/.platformio/penv/bin/pio run -e m5stack-core2-v1_0 -t upload
```

## Serial console
- Is included in evdash.eu M5 flasher https://www.evdash.eu/m5flash/
