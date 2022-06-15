CC=gcc
CDEFFLAGS=-std=c99 -Wall -Wextra -Wpedantic -Wconversion -Wdouble-promotion -Wstrict-prototypes
CDEBFLAGS=-g -O0
CFLAGS=-O3 -Wl,--strip-all,--build-id=none,--gc-sections
LIB=-lgdi32

SRC=src
APP=mouseStats


SRCS=$(wildcard $(SRC)/*.c)

default: debug

debug: $(SRCS)
	$(CC) $^ -o $(APP).d.exe $(CDEFFLAGS) $(CDEBFLAGS) $(LIB)

release: $(SRCS)
	$(CC) $^ -o $(APP) $(CDEFFLAGS) $(CFLAGS) $(LIB)

clean:
	del /Q $(APP)*.exe
