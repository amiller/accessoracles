all: ./swapout cprograms/gpt1

cprograms/gpt1: cprograms/gpt1.c
	gcc -o cprograms/gpt1 cprograms/gpt1.c

./swapout: swapout.c
	gcc -o swapout swapout.c

clean:
	rm -f ./swapout cprograms/gpt1

