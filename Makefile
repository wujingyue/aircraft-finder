CXX = g++
CXXFLAGS = -std=c++14 -g -Wall -Werror -O2 -pthread

all: aircraft_finder aircraft_generator

aircraft_placer.o: aircraft_placer.cc aircraft_placer.h color.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

aircraft_finder.o: aircraft_finder.cc aircraft_finder.h aircraft_placer.h color.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

aircraft_finder: aircraft_finder_main.cc aircraft_finder.o aircraft_placer.o
	$(CXX) $(CXXFLAGS) $^ -o $@

aircraft_generator.o: aircraft_generator.cc aircraft_generator.h aircraft_placer.h color.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

aircraft_generator: aircraft_generator_main.cc aircraft_generator.o aircraft_placer.o
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f aircraft_finder aircraft_generator *.o *.stackdump

.PHONY: clean
