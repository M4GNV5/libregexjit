#pragma once

#include "regexjit.h"

//flags <= 0xff are public and defined in regexjit.h
#define REGJIT_FLAG_MATCHES_START 0x100

typedef struct regjit_charset_range
{
	char min;
	char max;
	const struct regjit_charset_range *next;
} regjit_charset_range_t;
typedef struct regjit_charset
{
	bool inverted;
	bool lookupInitialized;
	const char *whitelist;
	const regjit_charset_range_t *ranges;
	uint8_t lookup[256];
} regjit_charset_t;

extern regjit_charset_t regjit_charset_all;
extern regjit_charset_t regjit_charset_digits;
extern regjit_charset_t regjit_charset_non_digits;
extern regjit_charset_t regjit_charset_whitespace;
extern regjit_charset_t regjit_charset_non_whitespace;
extern regjit_charset_t regjit_charset_word;
extern regjit_charset_t regjit_charset_non_word;

typedef struct regjit_expr_list regjit_expr_list_t;
typedef struct regjit_repeat regjit_repeat_t;

typedef struct
{
	enum
	{
		REGJIT_EXPR_CONST,
		REGJIT_EXPR_CHARSET,
		REGJIT_EXPR_OR,
		REGJIT_EXPR_LINE_START,
		REGJIT_EXPR_LINE_END,
		REGJIT_EXPR_WORD_BORDER,
		REGJIT_EXPR_REPEAT,
		REGJIT_EXPR_GROUP,
		REGJIT_EXPR_EXPRLIST,
	} kind;
	union
	{
		const char *literal;
		regjit_charset_t *charset;
		regjit_expr_list_t *body;
		regjit_repeat_t *repeat;
		bool inverted;
	} args;
} regjit_expression_t;

struct regjit_expr_list
{
	regjit_expression_t *expr;
	regjit_expr_list_t *next;
};

struct regjit_repeat
{
	regjit_expression_t *expr;
	size_t min;
	size_t max;
};

regjit_expression_t *regjit_parse(const char *expression);
