ARCHFLAGS=-mthumb -mcpu=cortex-m0plus
CFLAGS=-Os
LDFLAGS=--specs=nano.specs --specs=nosys.specs -Wl,--gc-sections,-Map,$(TARGET).map,-Tlink.ld
INC=./src ./src/freescale_usb ./src/freescale_usb/usb ./src/freescale_usb/bsp ./libs/nocli/src
INC_PARAMS=$(foreach d, $(INC), -I$d)
CFLAGS+=$(INC_PARAMS)

CC=arm-none-eabi-gcc
LD=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy
SIZE=arm-none-eabi-size
RM=rm -f

TARGET=main
SRC=$(wildcard *.c) $(wildcard src/*.c) $(wildcard libs/nocli/src/*.c) $(wildcard src/freescale_usb/*.c) $(wildcard src/freescale_usb/usb/*.c) $(wildcard src/freescale_usb/bsp/*.c)
OBJ=$(patsubst %.c, %.o, $(SRC))

all: build size
build: elf hex
elf: $(TARGET).elf
hex: $(TARGET).hex

clean:
	$(RM) $(TARGET).hex $(TARGET).elf $(TARGET).map $(OBJ)

%.o: %.c
	$(CC) -c $(ARCHFLAGS) $(CFLAGS) -o $@ $<

$(TARGET).elf: $(OBJ)
	echo $(SRC)
	$(LD) $(ARCHFLAGS) $(LDFLAGS) -o $@ $(OBJ)

%.hex: %.elf
	$(OBJCOPY) -O ihex $< $@

size:
	$(SIZE) $(TARGET).elf
