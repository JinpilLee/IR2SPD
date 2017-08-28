CXXFLAGS=-std=c++11 $(shell llvm-config --cxxflags)

#LDFLAGS=$(shell llvm-config --ldflags --system-libs --libs)
#-fno-rtti

all:
	@echo build pass...
	@clang++ -c $(CXXFLAGS) main.cpp -o main.o
	@clang++ -shared -fPIC main.o -o IR2SPD.so
	@echo build done

src: sample.c
	clang -cc1 -std=c99 -O3 -S -emit-llvm sample.c -o sample.ll

try: src
	opt -load ./IR2SPD.so -generate-spd sample.ll -S > /dev/null

clean:
	@rm -f *.so *.o sample.ll *.spd
