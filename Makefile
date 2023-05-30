CC=clang

.PHONY: all
all: main test

.PHONY: main
main:
	if [ ! -d "build" ]; then mkdir build; fi
	cd build && cmake -GNinja .. && ninja

.PHONY: test
test: main
	if [ ! -d "gen" ]; then mkdir gen; fi
	for i in `ls res/*.bmp`; do \
		./check.sh $$i; \
	done
