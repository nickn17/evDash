#!/bin/bash

rm dist/m5stack-core/evDash.ino.bin
rm dist/m5stack-core2/evDash.ino.bin
rm dist/ttgo-t4-v13/evDash.ino.bin

cp .pio/build/m5stack-core/firmware.bin dist/m5stack-core/evDash.ino.bin
cp .pio/build/m5stack-core2/firmware.bin dist/m5stack-core2/evDash.ino.bin
cp .pio/build/ttgo-t4-v13/firmware.bin dist/ttgo-t4-v13/evDash.ino.bin

pause
