%{
#include "parser.tab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <vector>
#include <algorithm>
#include <iterator>
extern std::vector<char*> strTable;
extern std::map<char*, char*> aliases;
extern bool scanStart;
char* env;
char* buf;

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

bool checkMeta(char* name) {
    for (int i = 0; i < strlen(name); i++) {
        if (name[i] == '\\') {
            return true;
        }
    }
    return false;
}

char* fixText(char* name) {
    char* copy = strdup(name);
    int i = 0;
    while (copy[i] != '\0') {
        if (copy[i] == '\\') {
            for (int j = i; j < strlen(copy); j++) {
                char temp = copy[j + 1];
                copy[j + 1] = copy[j];
                copy[j] = temp;
            }
            i++;
        }
        i++;
    }
    return copy;
}

%}

%option noyywrap

%x EXPECT_ENV
%x EXPECT_SENV
%x EXPECT_WORD

CHAR [.:A-Za-z0-9!\/_*?\-]
META [|<>"\&]
QUOTE ["]
NOTQUOTE [^"\n{$]

%%

($\{) BEGIN(EXPECT_ENV);
<EXPECT_ENV>{CHAR}+ { if(getenv(yytext) == NULL) {return UNDEFINED;} else {env = strdup(getenv(yytext));} }
<EXPECT_ENV>"\n" { BEGIN(INITIAL); return UNDEFINED; } 
<EXPECT_ENV>"}" { BEGIN(INITIAL); for (int i = strlen(env) - 1; i >= 0; --i) {unput(env[i]);} free(env); }

<EXPECT_SENV>{CHAR}+ { if(getenv(yytext) == NULL) {return UNDEFINED;} else {env = strdup(getenv(yytext));} }
<EXPECT_SENV>"\n" { BEGIN(EXPECT_WORD); return UNDEFINED; } 
<EXPECT_SENV>"}" { BEGIN(EXPECT_WORD); for (int i = strlen(env) - 1; i >= 0; --i) {unput(env[i]);} free(env); }

{QUOTE} { BEGIN(EXPECT_WORD); buf = (char*)malloc(256 + 1); buf[0] = '\0'; }
<EXPECT_WORD>($\{) {BEGIN(EXPECT_SENV);}
<EXPECT_WORD>{NOTQUOTE}* { if(checkAlias(yytext) && scanStart == true) { 
                                char* yycopy = strdup(getAlias(yytext));
                                for (int i = strlen(yycopy) - 1; i >= 0; --i) {
                                    unput(yycopy[i]);
                                }
                                free(yycopy);
                            } 
                            else { 
                                scanStart = false;
                                strcat(buf, yytext);
                            } 
                        } //WORD for text enclosed in ""
<EXPECT_WORD>"$" { strcat(buf, yytext); }
<EXPECT_WORD>"\n" { BEGIN(INITIAL); return UNDEFINED; } 
<EXPECT_WORD>{QUOTE} { BEGIN(INITIAL); yylval.WORD = saveString(buf); free(buf); return WORD; }

(2>) return ERRDIR;
(2>&1) return ERRDIR2;

{CHAR}+(\\([$"<>&\\]{1}))* { if(checkMeta(yytext)) {
                                char* fixedText = fixText(yytext);
                                yylval.WORD = saveString(fixedText); free(fixedText); return WORD;
                            }
                            else if(checkAlias(yytext) && scanStart == true) { 
                                char* yycopy = strdup(getAlias(yytext));
                                for (int i = strlen(yycopy) - 1; i >= 0; --i) {
                                    unput(yycopy[i]);
                                }
                                free(yycopy);
                            } 
                            else { 
                                scanStart = false;
                                yylval.WORD = saveString(yytext); return WORD; 
                            } 
                        } //WORD for any other text

[ \t\r] {} //ignores whitesapce

"\n" return NEWLINE;

{META}	return (int) yytext[0]; //treats every meta character as a separate token of their own ascii value

<<EOF>> return END_OF_FILE;

. return UNDEFINED; //captures unrecognized characters (causes error in parser)
