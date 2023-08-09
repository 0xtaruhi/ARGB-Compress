CC=gcc
CXX=g++

.PHONY: all
all: main test

.PHONY: main
main:
	if [ ! -d "build" ]; then mkdir build; fi
	cd build && cmake -DCMAKE_CXX_COMPILER=${CC} -DCMAKE_CXX_COMPILER=${CXX} -DCMAKE_EXPORT_COMPILE_COMMANDS=1 .. && make -j4

.PHONY: test
test: main
	if [ ! -d "gen" ]; then mkdir gen; fi
	for i in `ls res/*.bmp`; do \
		./check.sh $$i; \
	done
