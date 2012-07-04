#!/bin/sh
set -e

case "$(uname)" in
Linux)
	LIB_DIR=./lib-linux
	LFLAGS="-lopencv_highgui"
;;
Darwin)
	LIB_DIR=./lib-osx
	LFLAGS="-framework Cocoa -lopencv_highgui"
;;
*) echo "Unknown platform '$(uname)'"; exit 1 ;;
esac

CFLAGS="-ggdb"
LFLAGS="$LFLAGS -ggdb"

SPEECH="SAM"
if [ "${SPEECH}" = "SAM" ]; then
	SPEECH_LIBS="-lsam"
	CFLAGS="${CFLAGS} -DUSE_SAM"
else
	SPEECH_LIBS="-lespeak"
fi


echo Compiling capture.cpp...
g++ capture.cpp -c $CFLAGS -I./include -o capture.o

echo Compiling srv.c...
gcc srv.c -c  $CFLAGS -I./include -o srv.o 

echo Compiling speech.c...
gcc speech.c -c $CFLAGS -I./include -o speech.o 

echo Compiling oswrap.c...
gcc oswrap.c -c $CFLAGS -I./include -o oswrap.o

echo Compiling utils.c...
gcc utils.c -c $CFLAGS -I./include -o utils.o

echo Linking...
g++ capture.o srv.o oswrap.o speech.o utils.o $LFLAGS -L"${LIB_DIR}" -lSDL -lSDLmain -lx264 -lswscale -lavutil -lrcplug_srv $SPEECH_LIBS -o bin/srv

echo Cleaning up...
rm *.o
#strip srv

echo Done!
