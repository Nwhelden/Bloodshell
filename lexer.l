%{
    #include "test.tab.h"  
%}

%%

[a-z] {yylval.CHARACTER = yytext[0]; return CHARACTER;}

[ \t\r\n]+ {} //ignore whitespace (or [[:space:]]+ {} )

. return UNDEFINED;
