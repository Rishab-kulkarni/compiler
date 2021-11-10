#!/bin/sh



g++ -w -std=c++14 -g -o parser *.cpp `llvm-config --cxxflags --ldflags --libs --libfiles --system-libs` 


#g++ -w -std=c++14 -g -o codegen.cpp `llvm-config --cxxflags --ldflags --libs --libfiles --system-libs`

#g++ -w -std=c++14 -g -o parser parser.cpp tokens.cpp main.cpp codegen.cpp `llvm-config --cxxflags --ldflags --libs --libfiles --system-libs`

#g++ -w -std=c++14 -g parser.cpp tokens.cpp main.cpp -o a `llvm-config --cxxflags --ldflags --libs --libfiles --system-libs` 

