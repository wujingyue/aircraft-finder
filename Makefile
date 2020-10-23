CXX = g++
CXXFLAGS = -std=c++14 -g -Wall -Werror -O2 -pthread

all: aircraft_finder aircraft_generator benchmark

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

benchmark: benchmark.cc aircraft_generator.o aircraft_placer.o aircraft_finder.o
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f aircraft_finder aircraft_generator benchmark *.o *.stackdump

.PHONY: clean
