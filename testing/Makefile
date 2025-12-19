CC = gcc
CFLAGS = -g
TARGET = requests
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)
DEPS=$(wildcard *.h)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<;

build/$(TARGET): $(OBJECTS)
	$(CC) $^ -o $@;

.PHONY: all clean

all: build/$(TARGET)

clean:
	rm -f *.o
