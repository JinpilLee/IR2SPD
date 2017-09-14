CXXFLAGS=-std=c++11 -Wno-unknown-warning-option $(shell llvm-config --cxxflags)

all: IR2SPD.so CallGraphTest.so

IR2SPD.so: main.cpp
	@echo build pass: IR2SPD
	@clang++ -c $(CXXFLAGS) main.cpp -o main.o
	@clang++ -c $(CXXFLAGS) SPDPrinter.cpp -o SPDPrinter.o
	@clang++ -shared -fPIC main.o SPDPrinter.o -o IR2SPD.so
	@echo build done

CallGraphTest.so: CallGraphTest.cpp
	@echo build pass: CallGraphTest
	@clang++ -c $(CXXFLAGS) CallGraphTest.cpp -o CallGraphTest.o
	@clang++ -shared -fPIC CallGraphTest.o -o CallGraphTest.so
	@echo build done

src: sample.c
	clang -cc1 -std=c99 -O3 -fno-inline -S -emit-llvm sample.c -o sample.ll

print: src IR2SPD.so
	opt -load ./IR2SPD.so -generate-spd sample.ll -S > /dev/null

analysis: src CallGraphTest.so
	opt -load ./CallGraphTest.so -cgtest sample.ll -S > /dev/null

clean:
	@rm -f *.so *.o sample.ll *.spd *.cg
