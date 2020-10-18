CXX = g++
CXXFLAGS = -g -Wall -Werror -O2 -pthread

all: main

clean:
	rm -f main

.PHONY: clean
