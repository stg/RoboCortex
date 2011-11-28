#!/bin/sh
set -e

case "$(uname)" in
Linux)
	LIB_DIR=../lib-linux
;;
Darwin)
	LIB_DIR=../lib-osx
	LFLAGS="-framework Cocoa"
;;
*) echo "Unknown platform '$(uname)'"; exit 1 ;;
esac

mkdir -p bin
mkdir -p "${LIB_DIR}"

echo Compiling sam.c...
gcc sam.c -c -I ../include/sdl -I ./include

echo Compiling sammain.c...
gcc sammain.c -c -I ../include/sdl -I ./include

echo Linking...
gcc $LFLAGS sam.o sammain.o -lSDLmain -lSDL -o bin/sam

echo Librarian...
ar rcs "${LIB_DIR}"/libsam.a sam.o

echo Cleaning up...
rm *.o
#sleep 1
#strip bin/sam.exe

echo Done!

