# Working
> [!NOTE]
> This code is written using PlatformIO for ESP32 using the Arduino framework. Future maintainer are also recommended to use PlatformIO for better sharing and maintainability of the code.

There are three `.cpp` files in `src/`.
1. `load_test.cpp`: this just reads the raw load cell value and prints the data over serial port. *Can be used for **calibrating** the load cell.*
2. `main_eighty.cpp`: Use this if the *HX711* is set to 80 samples/second. This code reads and send the data over TCP at 80 samples/second. Also the ESP32 is connected to **6 MAX6675** thermocouple ADCs for temperature measurements. If no thermocouple is connected, the code will work jus fine and the readings will be `nan`.
3. `main.cpp`: use this if the HX711 is set to 10 samples/second. Also the ESP32 is connected to **6 MAX6675** thermocouple ADCs for temperature measurements. If no thermocouple is connected, the code will work jus fine and the readings will be `nan`.

To complie and upload either one of the files, open `platformio.ini` and change `<flag>` in `build_flags = -D <flag>`
| File              | Flag        |
|-------------------|-------------|
| `main.cpp`        | `TEN`       |
| `main_eighty.cpp` | `EIGHTY`    |
| `load_test.cpp`   | `LOAD_TEST` |

Example: To compile `main_eighty.cpp` set `build_flags = -D EIGHTY`