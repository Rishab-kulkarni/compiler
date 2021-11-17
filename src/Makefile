CC = g++
BISON = bison
FLEX = flex

CFLAGS = -g -w -std=c++14
LLVM_FLAGS = `llvm-config --cxxflags --ldflags --libs --libfiles --system-libs`
BISON_FLAGS = -d --warnings=none

parser.hpp:
	$(BISON) $(BISON_FLAGS) -o parser.cpp parser.y
	
tokens.cpp:
	$(FLEX) -o tokens.cpp tokens.l

run:
	$(CC) $(CFLAGS) -o parser *.cpp $(LLVM_FLAGS)    

clean:
	rm -rf tokens.cpp parser.cpp parser.hpp parser output

test:
	./parser < ../tests/$(test)
