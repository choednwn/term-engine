# compiler
CC = clang
CFLAGS = -std=c11 -Wall 

SRC = $(shell find . -name "*.c")

OBJ = $(SRC:.c=.o)
OUT = termengine.a

.PHONY: all clean
all: build 

%.o: %.c termengine.h
	$(CC) -o $@ -c $< $(CFLAGS) 

build: $(OBJ)
	ar -cr $(OUT) $(filter %.o, $^)
	rm -rf $(OBJ)

clean:
	rm -rf $(OBJ) $(OUT)