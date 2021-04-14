%{
#include "parser.tab.h"
#include <stdio.h>
#include <string.h>
#include <map>
#include <vector>
extern std::vector<char*> strTable;
extern std::map<char*, char*> aliases;
extern bool scanStart;

char* saveString(char* text) {
    char* temp = strdup(text);
    strTable.push_back(temp);
    return temp;
}

bool checkAlias(char* name) {
    std::map<char*, char*>::iterator iter;
    for (iter = aliases.begin(); iter != aliases.end(); iter++) {
        if(strcmp(iter->first, name) == 0) {
            return true;
        }
    }

    return false;
}

char* getAlias(char* name) {
    char* value;
    std::map<char*, char*>::iterator iter;
    for (iter = aliases.begin(); iter != aliases.end(); iter++) {
        if(strcmp(iter->first, name) == 0) {
            value = iter->second;
        }
    }
    return value;
}

%}

%option noyywrap

%x EXPECT_ENV
%x EXPECT_WORD

CHAR [.A-Za-z0-9!\/_-]
META [|<>"\&]
QUOTE ["]
NOTQUOTE [^"]

%%

($\{) BEGIN(EXPECT_ENV);
<EXPECT_ENV>{CHAR}+ { yylval.ENV = getenv(yytext); return ENV;}
<EXPECT_ENV>[}] BEGIN(INITIAL);

{QUOTE} BEGIN(EXPECT_WORD);
<EXPECT_WORD>{NOTQUOTE}* { yylval.WORD = saveString(yytext); return WORD; } //WORD for text enclosed in ""
<EXPECT_WORD>{QUOTE} BEGIN(INITIAL);

{CHAR}+(\\([$"<>&]{1}))* { if(checkAlias(yytext)) { 
                                char* yycopy = strdup(getAlias(yytext));
                                for (int i = strlen(yycopy) - 1; i >= 0; --i) {
                                    unput(yycopy[i]);
                                }
                                free(yycopy);
                                scanStart = false;
                            } 
                            else { 
                                yylval.WORD = saveString(yytext); return WORD; 
                                scanStart = false;
                            } 
                        } //WORD for any other text

[ \t\r] {} //ignores whitesapce

"\n" return NEWLINE;

{META}	return (int) yytext[0]; //treats every meta character as a separate token of their own ascii value

. return UNDEFINED; //captures unrecognized characters (causes error in parser)