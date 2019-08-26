#pragma once

#include "regexjit.h"

struct regjit_charset
{
	wchar_t min;
	wchar_t max;
	struct regjit_charset *next;
};

typedef struct regjit_expr_list regjit_expr_list_t;
typedef struct regjit_repeat regjit_repeat_t;

typedef struct
{
	enum
	{
		REGJIT_EXPR_CONST,
		REGJIT_EXPR_CHARSET,
		REGJIT_EXPR_OR,
		REGJIT_EXPR_REPEAT,
		REGJIT_EXPR_GROUP,
	} kind;
	union
	{
		const char *literal;
		struct regjit_charset *charset;
		regjit_expr_list_t *body;
        regjit_repeat_t *repeat;
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
