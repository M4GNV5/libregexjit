%{
#include <stdio.h>
#include "regexjit-internal.h"
#include "parser.h"
%}

literal			([^\|\*\+\?\(\)\[\]\.]|\\.)+
range			\{[0-9]+,?[0-9]*\}

%%

"|"				return(OR);
"*"				return(REPEAT_ANY);
"+"				return(REPEAT_AT_LEAST_ONCE);
"?"				return(REPEAT_AT_MOST_ONCE);
"("				return(GROUP_OPEN);
")"				return(GROUP_CLOSE);
"["				return(CHARSET_OPEN);
"]"				return(CHARSET_CLOSE);
"."				return(CHARSET_ALL);
"\\d"			return(CHARSET_DIGITS);
"\\D"			return(CHARSET_NON_DIGITS);
"\\s"			return(CHARSET_WHITESPACE);
"\\S"			return(CHARSET_NON_WHITESPACE);
"\\w"			return(CHARSET_ALPHANUMERIC);
"\\w"			return(CHARSET_NON_ALPHANUMERIC);

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
	//TODO handle escapes
	strcpy((char *)yylval.literal, yytext);
	return(LITERAL);
}

%%

regjit_expression_t *regjit_parse(const char *expression)
{
	regjit_expression_t *result = NULL;

	YY_BUFFER_STATE buffer = yy_scan_string(expression);

	if(yyparse(&result))
		result = NULL;

	yy_delete_buffer(buffer);
	return result;
}
