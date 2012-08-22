F_CPU=16000000
VARIANT=standard
MCU=atmega328p
#MCU=atmega168
TARGET=kiwiray
AVRDUDE_PORT=/dev/ttyACM0

CXX=avr-g++
CC=avr-gcc

COMMON_FLAGS += -IWire -IWire/utility

CXX_SRC+=Wire/Wire.cpp 

SRC += Wire/utility/twi.c 

kiwiray.o: KiwiRay.ino

include make.arduino
include make.core

