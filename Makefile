CXX = g++
CXXFLAGS = -std=c++14 -g -Wall -Werror -O2 -pthread

all: main

aircraft_placer.o: aircraft_placer.cc aircraft_placer.h color.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

main.o: main.cc aircraft_placer.h color.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

main: main.o aircraft_placer.o
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f main *.o

.PHONY: clean
