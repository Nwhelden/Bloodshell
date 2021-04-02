all:
	flex lexer.l
	bison -d parser.y
	g++ -o nutshell nutshell.cpp lex.yy.c parser.tab.c

clean:
	rm nutshell lex.yy.c parser.tab.c parser.tab.h

