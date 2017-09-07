CXXFLAGS=-std=c++11 $(shell llvm-config --cxxflags)

#LDFLAGS=$(shell llvm-config --ldflags --system-libs --libs)
#-fno-rtti

all: IR2SPD.so CallGraphTest.so

IR2SPD.so: main.cpp
	@echo build pass: IR2SPD
	@clang++ -c $(CXXFLAGS) main.cpp -o main.o
	@clang++ -shared -fPIC main.o -o IR2SPD.so
	@echo build done

CallGraphTest.so: CallGraphTest.cpp
	@echo build pass: CallGraphTest
	@clang++ -c $(CXXFLAGS) CallGraphTest.cpp -o CallGraphTest.o
	@clang++ -shared -fPIC CallGraphTest.o -o CallGraphTest.so
	@echo build done

src: sample.c
	clang -cc1 -std=c99 -O3 -S -emit-llvm sample.c -o sample.ll

try: src IR2SPD.so
	opt -load ./IR2SPD.so -generate-spd sample.ll -S > /dev/null

see: src CallGraphTest.so
	opt -load ./CallGraphTest.so -cgtest sample.ll -S > /dev/null

clean:
	@rm -f *.so *.o sample.ll *.spd *.cg
