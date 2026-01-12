
.PHONY: all compile run clean debug

all: run

compile:
	cc -g main.c

run: compile
	./a.out

debug: compile
	gdb ./a.out

clean:
	rm a.out

