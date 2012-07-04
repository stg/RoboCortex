all:
	./rc-make.sh

install:
	install -D bin/srv $(DESTDIR)/usr/bin/robocortex-server
	install -D bin/cli $(DESTDIR)/usr/bin/robocortex-cli
	cp -r etc $(DESTDIR)/etc
