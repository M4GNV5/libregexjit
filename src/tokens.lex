%{
#include "regexjit-internal.h"
#include "parser.tab.h"
%}

literal			[_a-zA-Z0-9]+

%%

"("				return(GROUP_OPEN);
")"				return(GROUP_CLOSE);
"["				return(CHARSET_OPEN);
"]"				return(CHARSET_CLOSE);
"{"				return(REPEAT_OPEN);
"}"				return(REPEAT_CLOSE);

{literal} {
	yylval.literal = malloc(strlen(yytext) + 1);
	strcpy((char *)yylval.literal, yytext);
	return(LITERAL);
}
