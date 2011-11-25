#!/bin/sh

SPEECH="ESPEAK"
if [ "${SPEECH}" = "SAM" ]; then
	SPEECH_LIBS="-lsam"
	CFLAGS="${CFLAGS} -DUSE_SAM"
else
	SPEECH_LIBS="-lespeak"
fi

echo Compiling cli.c...
gcc cli.c -c $CFLAGS -I./include -o cli.o

echo Compiling cli_term.c...
gcc cli_term.c -c $CFLAGS -I./include -o cli_term.o

echo Compiling oswrap.c...
gcc oswrap.c -c $CFLAGS -I./include -o oswrap.o

echo Compiling srv.c...
gcc speech.c -c $CFLAGS -I./include -o speech.o

echo Compiling utils.c...
gcc utils.c -c $CFLAGS -I./include -o utils.o

echo Linking...
gcc cli.o oswrap.o cli_term.o speech.o utils.o -L./lib-linux -l SDL -l avcodec -l avutil -l swscale -lz -lrcplug_cli $SPEECH_LIBS -o bin/cli

echo Cleaning up...
rm *.o
#strip cli

echo Done!
