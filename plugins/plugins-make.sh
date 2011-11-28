#!/bin/sh
set -e

case "$(uname)" in
Linux) LIB_DIR=../lib-linux;;
Darwin) LIB_DIR=../lib-osx;;
*) echo "Unknown platform '$(uname)'"; exit 1 ;;
esac

CFLAGS="-ggdb"

mkdir -p "${LIB_DIR}"

(
	# Need to delete old binaries otherwise compilation might fail on osx
	set +e
	rm "${LIB_DIR}/librcplug_cli.a" > /dev/null 2>&1
	rm "${LIB_DIR}/librcplug_srv.a" > /dev/null 2>&1
	exit 0
)

echo Compiling plugins/ipv4udp/...
gcc ipv4udp/srv.c -c $CFLAGS -I../include -I. -o ipv4udp_srv.o
gcc ipv4udp/cli.c -c $CFLAGS -I../include -I. -o ipv4udp_cli.o

echo Compiling plugins/kiwiray/...
gcc kiwiray/srv.c -c $CFLAGS -I../include -I. -o kiwiray_srv.o
gcc kiwiray/cli.c -c $CFLAGS -I../include -I. -o kiwiray_cli.o

echo Compiling plugins/monitor/...
gcc monitor/srv.c -c $CFLAGS -I../include -I. -o monitor_srv.o
gcc monitor/cli.c -c $CFLAGS -I../include -I. -o monitor_cli.o

echo Librarian...
ar rcs "${LIB_DIR}"/librcplug_srv.a ipv4udp_srv.o kiwiray_srv.o monitor_srv.o
ar rcs "${LIB_DIR}"/librcplug_cli.a ipv4udp_cli.o kiwiray_cli.o monitor_cli.o

echo Cleaning up...
rm *.o
#strip "${LIB_DIR}"/librcplug_srv.a
#strip "${LIB_DIR}"/librcplug_cli.a

echo Done!
