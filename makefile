.PHONY: all compile run clean

all: run

compile:
	cc main.c

run: compile
	./a.out

clean:
	rm a.out
