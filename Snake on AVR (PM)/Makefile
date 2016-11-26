all: main.hex

main.elf: main.cpp UTFT.cpp i2chw\twimastertimeout.c mpu6050\mpu6050.c mpu6050\mpu6050dmp6.c sdcard\pff.c sdcard\sd.c
	avr-g++ -mmcu=atmega324pa -DF_CPU=12000000 -O3 -Wall -lm -lc -Os -o $@ $^

main.hex: main.elf
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
	avr-size main.elf

clean:
	rm -rf main.elf main.hex

.PHONY: all clean