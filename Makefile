all:
	flex -d lexer.l
	bison -d -t parser.y
	g++ -o nutshell nutshell.cpp lex.yy.c parser.tab.c

clean:
	rm nutshell lex.yy.c parser.tab.c parser.tab.h
