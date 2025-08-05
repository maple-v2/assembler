assembler: assembler.o lexer.o parser.o preassembler.o utils.o
	gcc -g -ansi -Wall -pedantic  assembler.o lexer.o parser.o preassembler.o utils.o -o assembler
assembler.o : assembler.c globals.h 
	gcc -c -ansi -Wall assembler.c -o assembler.o
lexer.o : lexer.c globals.h  
	gcc -c -ansi -Wall lexer.c -o lexer.o
parser.o : parser.c globals.h  
	gcc -c -ansi -Wall parser.c -o parser.o
preassembler.o : preassembler.c globals.h
	gcc -c -ansi -Wall preassembler.c -o preassembler.o
utils.o : utils.c globals.h  
	gcc -c -ansi -Wall utils.c -o utils.o

clean: rm*~
