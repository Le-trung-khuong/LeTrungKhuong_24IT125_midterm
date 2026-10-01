CC = cc
CFLAGS = -Wall -Wextra -std=gnu99 -O2
CPPFLAGS = -Iinclude

TARGET = myls

OBJ = src/main.o src/options.o src/list.o src/display.o src/sort.o src/util.o

HDR = include/ls.h include/options.h include/list.h include/display.h \
      include/sort.h include/util.h

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ): $(HDR)

clean:
	rm -f $(OBJ) $(TARGET)

rebuild: clean all

# Debug build with AddressSanitizer and UBSan
debug:
	$(MAKE) clean
	$(MAKE) CFLAGS="-Wall -Wextra -std=gnu99 -g -O0 -fsanitize=address,undefined" all

test: all
	sh tests/run_tests.sh

.PHONY: all clean rebuild debug test
