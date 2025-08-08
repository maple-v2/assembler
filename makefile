assembler: assembler.o preassembler.o utils.o
	gcc -g -ansi -Wall -pedantic assembler.o preassembler.o utils.o -o assembler

assembler.o: assembler.c globals.h utils.h
	gcc -c -ansi -Wall -pedantic assembler.c -o assembler.o

preassembler.o: preassembler.c globals.h
	gcc -c -ansi -Wall -pedantic preassembler.c -o preassembler.o

utils.o: utils.c globals.h utils.h
	gcc -c -ansi -Wall -pedantic utils.c -o utils.o

.PHONY: clean
clean:
	rm -f *.o assembler
