CXX = g++
CXXFLAGS = -std=c++14 -g -Wall -Werror -O2 -pthread

all: main

clean:
	rm -f main

.PHONY: clean
