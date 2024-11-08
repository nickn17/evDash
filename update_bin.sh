#!/bin/bash

rm dist/m5stack-core2-v1_0/evDash.ino.bin
rm dist/m5stack-core2-v1_1/evDash.ino.bin
rm dist/m5stack-cores3/evDash.ino.bin

cp .pio/build/m5stack-core2-v1_0/firmware.bin dist/m5stack-core2-v1_0/evDash.ino.bin
cp .pio/build/m5stack-core2-v1_1/firmware.bin dist/m5stack-core2-v1_1/evDash.ino.bin
cp .pio/build/m5stack-cores3/firmware.bin dist/m5stack-cores3/evDash.ino.bin

