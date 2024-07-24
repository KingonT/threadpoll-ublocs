SRC=testPoll.cpp

Target=testBin

CC=g++

STAND=-std=c++17

CFLAGS=-I ./

LDFLAGS=-ltdpool -lpthread

$(Target):$(SRC)
	$(CC) $^  $(LDFLAGS)  -o $@ $(STAND)

.PHONY:clean
clean:
	rm -rf $(Target)
