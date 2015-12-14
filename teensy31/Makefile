
# The name of your project (used to name the compiled .hex file)
TARGET = btCSW_teensy31
TARGETPATH = ../firmware

# configurable options
OPTIONS = -DF_CPU=24000000 -DLAYOUT_US_ENGLISH -DUSB_DISABLED

# options needed by many Arduino libraries to configure for Teensy 3.1
OPTIONS += -D__MK20DX256__ -DARDUINO=166 -DTEENSYDUINO=126 

# Other Makefiles and project templates for Teensy 3.x:
#
# https://github.com/apmorton/teensy-template
# https://github.com/xxxajk/Arduino_Makefile_master
# https://github.com/JonHylands/uCee


#************************************************************************
# Location of Teensyduino utilities, Toolchain, and Arduino Libraries.
# To use this makefile without Arduino, copy the resources from these
# locations and edit the pathnames.  The rest of Arduino is not needed.
#************************************************************************

COREPATH = ../lib

# path location for the arm-none-eabi compiler
COMPILERPATH = /usr/bin

#************************************************************************
# Settings below this point usually do not need to be edited
#************************************************************************

# CPPFLAGS = compiler options for C and C++
CPPFLAGS = -Wall -ffunction-sections -fdata-sections -g -Os -mcpu=cortex-m4 -mthumb -nostdlib -MMD $(OPTIONS) -I. -I$(COREPATH) 

# compiler options for C++ only
CXXFLAGS = -std=gnu++0x -felide-constructors -fno-exceptions -fno-rtti

# compiler options for C only
CFLAGS =

# linker options
LDFLAGS = -Os -Wl,--gc-sections,--relax -mcpu=cortex-m4 -mthumb -T$(COREPATH)/mk20dx256.ld

# additional libraries to link
LIBS = -lm

# names for the compiler programs
CC = $(COMPILERPATH)/arm-none-eabi-gcc
CXX = $(COMPILERPATH)/arm-none-eabi-g++
OBJCOPY = $(COMPILERPATH)/arm-none-eabi-objcopy
SIZE = $(COMPILERPATH)/arm-none-eabi-size

# automatically create lists of the sources and objects
C_FILES := $(wildcard *.c)
CPP_FILES := $(wildcard *.cpp)
# automatically create lists of the sources and objects
TC_FILES := $(wildcard $(COREPATH)/*.c)
TCPP_FILES := $(wildcard $(COREPATH)/*.cpp)
OBJS := $(C_FILES:.c=.o) $(CPP_FILES:.cpp=.o) $(TC_FILES:.c=.o) $(TCPP_FILES:.cpp=.o)

# the actual makefile rules (all .o files built by GNU make's default implicit rules)

all: $(TARGET).hex

$(TARGET).elf: $(OBJS) $(COREPATH)/mk20dx256.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.hex: %.elf
	$(SIZE) $<
	$(OBJCOPY) -O ihex -R .eeprom $< $(TARGETPATH)/$@


upload: $(TARGET).hex
	teensy_loader_cli -mmcu=mk20dx256 -w -v $(TARGETPATH)/$@
	# teensy_post_compile -file=$(TARGET) -path=$(shell pwd) -tools=$(abspath $(TOOLSPATH))
	# -teensy_reboot

# compiler generated dependency info
-include $(OBJS:.o=.d)

clean:
	rm -f $(TARGET).elf $(TARGETPATH)/$(TARGET).hex
	rm -f $(OBJS) $(OBJS:.o=.d)


