CFLAGS=-Wall -ggdb -std=c11 -pedantic -lm

all: pixel_converter.c
	$(CC) $(CFLAGS) pixel_converter.c -o pixel_converter

clean: rm -f pixel_converter
