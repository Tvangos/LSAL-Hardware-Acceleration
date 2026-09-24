CXX = g++
CXXFLAGS = -Wall -g -O3 -mavx2 -fopenmp

TARGETS = target/lsal_orig target/lsal_opt1 target/lsal_opt2 target/lsal_opt3 target/lsal_opt4

UTILS = lab_input/utils.cpp
DEPS = lab_input/lsal.h

all: $(TARGETS)

target/%: lab_input/%.cpp $(UTILS) $(DEPS) | target
	$(CXX) $(CXXFLAGS) -o $@ $< $(UTILS)

target:
	mkdir -p target

clean:
	rm -rf target

.PHONY: all clean