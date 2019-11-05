all: joinbar

install: all
	install -m 755 -s -D joinbar ${DESTDIR}${PREFIX}/usr/bin/joinbar

joinbar: joinbar.c
	cc -Wall -Wextra -pedantic -std=c99 -g -ljson-c $< -o $@
