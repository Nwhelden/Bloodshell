%{
#include "parser.tab.h"
#include <vector>
extern std::vector<char*> strTable;

char* saveString(char* text) {
    char* temp = strdup(text);
    strTable.push_back(temp);
    return temp;
}

%}

%option noyywrap

%x EXPECT_STRING

META	[\|<>]

QUOTE		["]
NOTQUOTE	[^"]

%%

{QUOTE} BEGIN(EXPECT_STRING);
<EXPECT_STRING>{NOTQUOTE}* { yylval.WORD = saveString(yytext); return WORD; } //WORD is text enclosed in ""
<EXPECT_STRING>{QUOTE} BEGIN(INITIAL);

[a-zA-Z0-9_]+ { yylval.NAME = saveString(yytext); return NAME; } //NAME is any other text

[ \t\r] {} //ignores whitesapce

"\n" return NEWLINE;

{META}	return (int) yytext[0]; //treats every meta character as a separate token of their own ascii value

. return UNDEFINED; //captures unrecognized characters (causes error in parser)
