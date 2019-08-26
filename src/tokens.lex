%{
#include <stdio.h>
#include "regexjit-internal.h"
#include "parser.h"
%}

literal			[_a-zA-Z0-9]+
range			\{\d+(,(\d+)?)?\}

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

{range} {
	char *end;
	yylval.repetition = malloc(sizeof(regjit_repeat_t));
	yylval.repetition->min = strtoll(yytext + 1, &end, 10);

	if(end[0] == '}')
		yylval.repetition->max = yylval.repetition->min;
	else if(end[0] == ',' && end[1] == '}')
		yylval.repetition->max = SIZE_MAX;
	else
		yylval.repetition->max = strtoll(end + 1, NULL, 10);

	return(REPEAT_RANGE);
}
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
