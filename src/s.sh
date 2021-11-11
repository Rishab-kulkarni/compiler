#!/bin/sh



#g++ -w -std=c++14 -g -o parser *.cpp `llvm-config --cxxflags --ldflags --libs --libfiles --system-libs` 

g++ -w -std=c++14 -g -o a kaleidoscope.cpp `llvm-config --cxxflags --ldflags --libs --libfiles --system-libs` 



