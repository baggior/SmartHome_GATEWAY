C:\Users\Marco\.platformio\packages\framework-arduinoespressif32\tools\mkspiffs\mkspiffs.exe -c ./data -p 256 -b 4096 -s 1503232 my.spiffs.bin

C:\Users\Marco\.platformio\packages\framework-arduinoespressif32\tools\esptool.exe --chip esp32 --baud 921600 --port COM3 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_size detect 2691072 ./my.spiffs.bin