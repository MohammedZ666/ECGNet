default:
	avr-g++ -Os -Wall -ffunction-sections -fdata-sections -DF_CPU=16000000L -mmcu=atmega328p -std=c++11 -c -o build/main.o main.cpp
	avr-g++ -Os -mmcu=atmega328p -ffunction-sections -fdata-sections -Wl,--gc-sections -o build/main.bin build/main.o
	avr-objcopy -O ihex -R .eeprom build/main.bin build/main.hex
	avrdude -F -v -v -c arduino -p ATMEGA328P -P /dev/ttyACM0 -b 115200 -U flash:w:build/main.hex
