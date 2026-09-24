CXX = /tools/Xilinx/Vitis/2023.2/gnu/aarch32/lin/gcc-arm-linux-gnueabi/bin/arm-linux-gnueabihf-g++
CXXFLAGS = -Wall -O3 -g -std=c++11 -static

TARGETS = target/lsal target/lsal_opt1 target/lsal_opt2 target/lsal_opt3

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