set fw=framework-arduinoespressif32@src-537c58760dafe7fcc8a1d9bbcf00b6f6
rem set fw=framework-arduinoespressif32

C:\Users\Marco\.platformio\packages\%fw%\tools\mkspiffs\mkspiffs.exe -c ./data -p 256 -b 4096 -s 1503232 my.spiffs.bin

C:\Users\Marco\.platformio\packages\%fw%\tools\esptool.exe --chip esp32 --baud 921600 --port COM3 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_size detect 2691072 ./my.spiffs.bin