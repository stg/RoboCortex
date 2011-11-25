#!/bin/sh

CFLAGS="-ggdb"
LFLAGS="-ggdb"

SPEECH="ESPEAK"
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
g++ capture.o srv.o oswrap.o speech.o utils.o $LFLAGS -L./lib-linux -lSDL -lcv -lhighgui -lx264 -lswscale -lavutil -lcv -lrcplug_srv $SPEECH_LIBS -o bin/srv

echo Cleaning up...
rm *.o
#strip srv

echo Done!
