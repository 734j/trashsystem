CC=g++
CFLAGS_TESTBIN=-O0 -Wfatal-errors -Wall -Werror -Wextra -g3 -fsanitize=address -fsanitize=leak -Wpedantic -Wformat=2 -Wshadow -Wformat-truncation=2 -Wformat-overflow -fno-common -std=c++20
CFLAGS=-O3 -flto -march=native -DNDEBUG -fomit-frame-pointer -s -static -std=c++20
TARGET=tsr
TESTTARGET=tsr-TESTING
SP_TESTTARGET=tsr-SP
INSTALL_DIRECTORY=/usr/local/bin
MAKEFLAGS += 
SRCS=trashsys.cc
SRCS_SP=trashsys_small_paths.cc
#P_MAX_SIZE="47"

all: release
clean:
	rm -f $(TARGET)
	rm -f test/$(TESTTARGET)
	rm -f test/$(SP_TESTTARGET)
	rm -f $(SRCS_SP)

tests:
	cp $(SRCS) $(SRCS_SP)
	$(CC) $(CFLAGS_TESTBIN) $(SRCS) -o test/$(TESTTARGET)
	#sed -i "s#PATH_MAX#$(P_MAX_SIZE)#g" $(SRCS_SP)
	#$(CC) $(CFLAGS_TESTBIN) $(SRCS_SP) -o test/$(SP_TESTTARGET)
	#rm -f $(SRCS_SP)

install:
	cp $(TARGET) $(INSTALL_DIRECTORY)

release:
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)
