F_CPU=16000000
VARIANT=standard
MCU=atmega328p
#MCU=atmega168
TARGET=kiwiray

CXX=avr-g++
CC=avr-gcc

COMMON_FLAGS += -IWire -IWire/utility

CXX_SRC+=Wire/Wire.cpp 

SRC += Wire/utility/twi.c 


include make.arduino
include make.core

