#!/bin/sh
set -e

case "$(uname)" in
Linux)
	LIB_DIR=./lib-linux
;;
Darwin)
	LIB_DIR=./lib-osx
	LFLAGS="-framework Cocoa"
;;
*) echo "Unknown platform '$(uname)'"; exit 1 ;;
esac

SPEECH="SAM"
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
gcc $LFLAGS cli.o oswrap.o cli_term.o speech.o utils.o -L"${LIB_DIR}" -lSDL -lSDLmain -lavcodec -lavutil -lswscale -lz -lrcplug_cli $SPEECH_LIBS -o bin/cli

echo Cleaning up...
rm *.o
#strip cli

echo Done!
