
MICROCONTROLLER = atmega328p
PROGRAMMER = atmelice_isp
PROGRAMMER_TARGET = m328p
PROGRAMMER_BITCLOCK = 1

# Source directory.
SRC_DIR = ./firmware/src
# Include directory.
INC_DIR = ./firmware/inc

# List of source files.
SRC := $(wildcard $(SRC_DIR)/*.c)

# List of object files.
# NOTE: The object files are in the same directory as the source files.
OBJ := $(subst .c,.o,$(SRC))

# The name of the compiled executable.
EXE = thermometer

# GCC specific flags
CFLAGS := -I $(INC_DIR) -O1

# AVR-GCC specific flags
AVR_CFLAGS := -mmcu=$(MICROCONTROLLER)
HFUSE_DEFAULT = 0xd9
HFUSE_DEBUG = 0x99

#default: program

$(EXE): $(OBJ)
	avr-gcc $(CFLAGS) $(AVR_CFLAGS) $(OBJ) -o $(EXE)
	avr-objcopy -O ihex $(EXE) $(EXE).hex
	sudo avrdude\
		-c $(PROGRAMMER)\
		-p $(PROGRAMMER_TARGET)\
		-B $(PROGRAMMER_BITCLOCK)\
		-U flash:w:$(EXE).hex



# Static pattern rules and Automatic pattern matching. Creates individual rules
# for each object and source file instead of having each object file depend on 
# every source file.
$(OBJ): $(SRC_DIR)/%.o : $(SRC_DIR)/%.c 
	avr-gcc $(CFLAGS) $(AVR_CFLAGS) -c $< -o $@

clean:
	find . -type f -name '*.o' -delete
	rm $(EXE) *.hex debug

debug:
	avr-gcc -g $(CFLAGS) $(AVR_CFLAGS) $(SRC) -o debug
	avr-objcopy -O ihex debug debug.hex
	sudo avrdude -c atmelice_isp -p m328p -B 1 -U flash:w:debug.hex
	sudo avrdude -c atmelice_isp -p m328p -B 1 -U hfuse:w:$(HFUSE_DEBUG):m
	echo "Power cycle the target, then press any key to continue..."
	read -s
	sudo avarice -4w :6969
	sudo avrdude -c atmelice_isp -p m328p -B 1 -U hfuse:w:$(HFUSE_DEFAULT):m

start:
	sudo avr-gdb -ex "target remote localhost:6969"

stop:
	sudo avrdude -c atmelice_isp -p m328p -B 1 -U hfuse:w:$(HFUSE_DEFAULT):m
