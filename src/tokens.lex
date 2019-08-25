%{
#include <stdio.h>
#include "regexjit-internal.h"
#include "parser.h"
%}

literal			[_a-zA-Z0-9]+

%%

"|"				return(OR);
"*"				return(REPEAT_ANY);
"+"				return(REPEAT_ANYPOSITIVE);
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

%%

regjit_expr_list_t *regjit_parse(const char *expression)
{
	regjit_expr_list_t *result = NULL;

	printf("parsing '%s'\n", expression);

	YY_BUFFER_STATE buffer = yy_scan_string(expression);

	if(yyparse(&result))
		result = NULL;

	yy_delete_buffer(buffer);
	return result;
}
