CC = g++
SRC_DIR = src/
INCLUDEPATH = -I src

MAINSRC = $(notdir $(shell ls $(SRC_DIR)*.cpp))
OBJ = $(patsubst %.cpp, %.o, $(MAINSRC))

CFLAGS = -O0 -g -fPIC -Werror

fblcd.out: $(OBJ)
	$(CC) $^ $(CFLAGS) -o $@

$(OBJ): %.o: $(SRC_DIR)%.cpp
	$(CC) -c $< $(INCLUDEPATH) $(CFLAGS) -o $@

.PHONY: clean
clean :
	rm -f *.out
	rm -f *.o
