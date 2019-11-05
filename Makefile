all: joinbar

joinbar: joinbar.c
	cc -Wall -Wextra -pedantic -std=c99 -g -ljson-c $< -o $@
