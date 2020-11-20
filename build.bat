arduino-cli compile --fqbn esp32:esp32:esp32 --build-properties build.extra_flags=-BOARD_TTGO_T4=1 -v enirodashboard.ino
arduino-cli compile 
REM arduino-cli upload -b esp32:esp32:esp32 -v -p COM6
pause