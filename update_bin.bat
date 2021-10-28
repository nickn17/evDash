del dist\m5stack-core\evDash.ino.bin
del dist\m5stack-core2\evDash.ino.bin
del dist\ttgo-t4-v13\evDash.ino.bin

copy .pio\build\m5stack-core\firmware.bin dist\m5stack-core\evDash.ino.bin
copy .pio\build\m5stack-core2\firmware.bin dist\m5stack-core2\evDash.ino.bin
copy .pio\build\ttgo-t4-v13\firmware.bin dist\ttgo-t4-v13\evDash.ino.bin

pause