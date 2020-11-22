
rem slow GUI performance via arduino-cli yet

rem arduino-cli compile -v -b esp32:esp32:esp32:PSRAM=enabled,PartitionScheme=huge_app,CPUFreq=80 --build-properties build.extra_flags=-BOARD_TTGO_T4=1 -v enirodashboard.ino
rem arduino-cli upload -b esp32:esp32:esp32 -v -p COM6

rem arduino-cli compile -v -b esp32:esp32:m5stack-core-esp32 --build-properties build.extra_flags=-BOARD_M5STACK=1 enirodashboard.ino
rem arduino-cli upload -v -b esp32:esp32:m5stack-core-esp32 -p COM9

rem arduino-cli compile -v -b esp32:esp32:esp32:PSRAM=enabled,PartitionScheme=huge_app,CPUFreq=80 --build-properties build.extra_flags=-BOARD_M5STACK=1 -v enirodashboard.ino
rem arduino-cli upload -b esp32:esp32:esp32 -v -p COM9

pause