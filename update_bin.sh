#!/bin/bash

rm dist/m5stack-core2/evDash.ino.bin
rm dist/m5stack-cores3/evDash.ino.bin

cp .pio/build/m5stack-core2/firmware.bin dist/m5stack-core2/evDash.ino.bin
cp .pio/build/m5stack-cores3/firmware.bin dist/m5stack-cores3/evDash.ino.bin

