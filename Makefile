default:
	avr-g++ -Os -Wall -ffunction-sections -fdata-sections -DF_CPU=16000000L -mmcu=atmega328p -c -o main.o main.cpp
	avr-g++ -Os -mmcu=atmega328p -ffunction-sections -fdata-sections -Wl,--gc-sections -o main.bin main.o
	avr-objcopy -O ihex -R .eeprom main.bin main.hex
	sudo avrdude -F -v -v -c arduino -p ATMEGA328P -P /dev/ttyACM0 -b 115200 -U flash:w:main.hex
