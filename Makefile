CC=gcc
CFLAGS_TESTBIN=-O3 -Wfatal-errors -Wall -Werror -Wextra -g -fsanitize=address -Wpedantic -std=gnu99
CFLAGS=-O3 -flto -march=native -DNDEBUG -fomit-frame-pointer -s -static -std=gnu99
TARGET=tsr
TESTTARGET=tsr-TESTING
INSTALL_DIRECTORY=/usr/local/bin
MAKEFLAGS += 
SRCS=trashsys.c

all: release
clean:
	rm -f $(TARGET)
	rm -f test/$(TESTTARGET)

tests:
	$(CC) $(CFLAGS_TESTBIN) $(SRCS) -o test/$(TESTTARGET)

install:
	cp $(TARGET) $(INSTALL_DIRECTORY)

release:
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)
