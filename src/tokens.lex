%{
#include <stdio.h>
#include "regexjit-internal.h"
#include "parser.h"
%}

literal			([^\|\^\$\*\+\?\(\)\[\]\{\}\.\\]|\\[^bBdDsSwW])+
charset			\[[^\]]+\]
range			\{[0-9]+,?[0-9]*\}

%%

"|"				return(OR);
"^"				return(LINE_START);
"$"				return(LINE_END);
"\\b"			return(WORD_BORDER);
"\\B"			return(NON_WORD_BORDER);
"*"				return(REPEAT_ANY);
"+"				return(REPEAT_AT_LEAST_ONCE);
"?"				return(REPEAT_AT_MOST_ONCE);
"(?:"			return(GROUP_OPEN_NOMATCH);
"("				return(GROUP_OPEN);
")"				return(GROUP_CLOSE);
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
{charset} {
	size_t len = strlen(yytext) - 2;
	char *str = malloc(len + 1);
	memcpy(str, yytext + 1, len);
	str[len] = 0;

	//TODO handle escapes
	yylval.literal = str;
	return(CHARSET_CUSTOM);
}
{literal} {
	size_t len = strlen(yytext) + 1;
	yylval.literal = malloc(len);
	//TODO handle escapes
	memcpy((char *)yylval.literal, yytext, len);
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
