CC      = gcc
TARGET  = banker
SRC     = banker.c

CFLAGS  = -std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200809L
LDFLAGS =
LDLIBS  =

.PHONY: all clean distclean rebuild

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

rebuild: clean all

clean:
	rm -f $(TARGET)

distclean: clean
	rm -f result.txt
