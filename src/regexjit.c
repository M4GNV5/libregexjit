#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jit/jit.h>
#include <jit/jit-dump.h>
#include "regexjit-internal.h"
#include "parser.h"
#include "tokens.h"

#define const(t, v) (jit_value_create_nint_constant(ctx->func, jit_type_##t, (v)))
#define constl(t, v) (jit_value_create_long_constant(ctx->func, jit_type_##t, (v)))

typedef struct
{
	jit_function_t func;
	jit_value_t str;
	jit_value_t strEnd;
	jit_label_t *noMatch;

	jit_value_t groups;
	size_t groupCount;
} regjit_compilation_t;

struct regjit_regex
{
	jit_function_t func;
	size_t groupCount;
};

void regjit_compile_expression(regjit_compilation_t *ctx, regjit_expression_t *expr);
void regjit_compile_expression_list(regjit_compilation_t *ctx, regjit_expr_list_t *exprs);

void regjit_compile_const(regjit_compilation_t *ctx, regjit_expression_t *expr)
{
	const char *literal = expr->args.literal;
	size_t len = strlen(literal);
	size_t offset = 0;
	jit_value_t tmp1;
	jit_value_t tmp2;

	if(len < 4)
	{
		while(len > 0)
		{
			tmp1 = jit_insn_load_relative(ctx->func, ctx->str, offset, jit_type_ubyte);
			tmp1 = jit_insn_eq(ctx->func, tmp1, const(ubyte, literal[offset]));
			jit_insn_branch_if_not(ctx->func, tmp1, ctx->noMatch);

			offset++;
			len--;
		}

		tmp1 = jit_insn_add(ctx->func, ctx->str, const(nuint, offset));
		jit_insn_store(ctx->func, ctx->str, tmp1);
		return;
	}

	//check if we would exceed the end of the string
	tmp1 = jit_insn_add(ctx->func, ctx->str, const(nuint, len));
	tmp1 = jit_insn_gt(ctx->func, tmp1, ctx->strEnd);
	jit_insn_branch_if(ctx->func, tmp1, ctx->noMatch);

	int sizes[] = {8, 4, 2, 1};
	jit_type_t types[] = {
		jit_type_ulong,
		jit_type_uint,
		jit_type_ushort,
		jit_type_ubyte,
	};

	for(int i = 0; i < 4; i++)
	{
		int size = sizes[i];
		jit_type_t type = types[i];

		while(len >= size)
		{
			tmp1 = jit_insn_load_relative(ctx->func, ctx->str, offset, type);

			if(size == 8)
				tmp2 = constl(ulong, *(uint64_t *)(literal + offset));
			else if(size == 4)
				tmp2 = const(nuint, *(uint32_t *)(literal + offset));
			else if(size == 2)
				tmp2 = const(nuint, *(uint16_t *)(literal + offset));
			else if(size == 1)
				tmp2 = const(nuint, *(uint8_t *)(literal + offset));

			tmp1 = jit_insn_eq(ctx->func, tmp1, tmp2);
			jit_insn_branch_if_not(ctx->func, tmp1, ctx->noMatch);

			len -= size;
			offset += size;
		}
	}

	tmp1 = jit_insn_add(ctx->func, ctx->str, const(nuint, offset));
	jit_insn_store(ctx->func, ctx->str, tmp1);
}

void regjit_compile_charset(regjit_compilation_t *ctx, regjit_expression_t *expr)
{
	jit_value_t tmp;
	const regjit_charset_t *charset = expr->args.charset;
	jit_value_t input = jit_insn_load_relative(ctx->func, ctx->str, 0, jit_type_ubyte);
	jit_label_t doesMatch = jit_label_undefined;

	tmp = jit_insn_eq(ctx->func, input, const(ubyte, 0));
	jit_insn_branch_if(ctx->func, tmp, ctx->noMatch);

	// special case for charsets that matches everything (i.e. '.')
	if(charset->ranges == NULL && charset->whitelist == NULL)
	{
		if(charset->inverted)
		{
			jit_insn_branch(ctx->func, ctx->noMatch);
		}
		else
		{
			tmp = jit_insn_add(ctx->func, ctx->str, const(nuint, 1));
			jit_insn_store(ctx->func, ctx->str, tmp);
		}

		return;
	}

	jit_label_t *foundMatchingChar;
	if(charset->inverted)
		foundMatchingChar = ctx->noMatch;
	else
		foundMatchingChar = &doesMatch;

	const regjit_charset_range_t *range = charset->ranges;
	for(; range != NULL; range = range->next)
	{
		tmp = jit_insn_sub(ctx->func, input, const(ubyte, range->min));
		tmp = jit_insn_le(ctx->func, tmp, const(ubyte, range->max - range->min));
		jit_insn_branch_if(ctx->func, tmp, foundMatchingChar);
	}

	if(charset->whitelist)
	{
		const char *whitelist = charset->whitelist;
		size_t len = strlen(whitelist);
		jit_value_t mask32;
		jit_value_t mask;

		if(len >= 8)
		{
			mask = jit_insn_convert(ctx->func, input, jit_type_ulong, 0);
			for(int i = 8; i <= 32; i *= 2)
			{
				tmp = jit_insn_shl(ctx->func, mask, constl(ulong, i));
				mask = jit_insn_or(ctx->func, tmp, mask);

				if(i == 16)
					mask32 = mask;
			}
		}
		else if(len >= 4)
		{
			mask32 = jit_insn_convert(ctx->func, input, jit_type_uint, 0);
			for(int i = 8; i <= 16; i *= 2)
			{
				tmp = jit_insn_shl(ctx->func, mask32, const(uint, i));
				mask32 = jit_insn_or(ctx->func, tmp, mask32);
			}
		}

		while(len >= 8)
		{
			jit_value_t chunk = constl(ulong, *(uint64_t *)whitelist);
			tmp = jit_insn_xor(ctx->func, chunk, mask);
			tmp = jit_insn_add(ctx->func, tmp, constl(ulong, 0x7f7f7f7f7f7f7f7f));

			jit_value_t holes = constl(ulong, 0x8080808080808080);
			tmp = jit_insn_and(ctx->func, tmp, holes);
			tmp = jit_insn_ne(ctx->func, tmp, holes);
			jit_insn_branch_if(ctx->func, tmp, foundMatchingChar);

			whitelist += 8;
			len -= 8;
		}

		while(len >= 4)
		{
			jit_value_t chunk = const(uint, *(uint32_t *)whitelist);
			tmp = jit_insn_xor(ctx->func, chunk, mask32);
			tmp = jit_insn_add(ctx->func, tmp, const(uint, 0x7f7f7f7f));

			jit_value_t holes = const(uint, 0x80808080);
			tmp = jit_insn_and(ctx->func, tmp, holes);
			tmp = jit_insn_ne(ctx->func, tmp, holes);
			jit_insn_branch_if(ctx->func, tmp, foundMatchingChar);

			whitelist += 4;
			len -= 4;
		}

		while(len > 0)
		{
			tmp = jit_insn_eq(ctx->func, input, const(uint, *whitelist));
			jit_insn_branch_if(ctx->func, tmp, foundMatchingChar);

			whitelist++;
			len--;
		}
	}

	if(!charset->inverted)
	{
		//we checked all chars in the whitelist but found no matching one
		// => the char does not match
		jit_insn_branch(ctx->func, ctx->noMatch);

		//we jump here when we successfully found a match
		jit_insn_label(ctx->func, &doesMatch);
	}

	tmp = jit_insn_add(ctx->func, ctx->str, const(nuint, 1));
	jit_insn_store(ctx->func, ctx->str, tmp);
}

void regjit_compile_group(regjit_compilation_t *ctx, regjit_expression_t *expr)
{
	jit_label_t capturingDisabled;
	size_t index = ctx->groupCount;
	ctx->groupCount++;

	size_t offset1 = index * sizeof(regjit_match_t);
	size_t offset2 = offset1 + offsetof(regjit_match_t, end);

	capturingDisabled = jit_label_undefined;
	jit_insn_branch_if_not(ctx->func, ctx->groups, &capturingDisabled);
	jit_insn_store_relative(ctx->func, ctx->groups, offset1, ctx->str);
	jit_insn_label(ctx->func, &capturingDisabled);

	regjit_compile_expression_list(ctx, expr->args.body);

	capturingDisabled = jit_label_undefined;
	jit_insn_branch_if_not(ctx->func, ctx->groups, &capturingDisabled);
	jit_insn_store_relative(ctx->func, ctx->groups, offset2, ctx->str);
	jit_insn_label(ctx->func, &capturingDisabled);
}

void regjit_compile_or(regjit_compilation_t *ctx, regjit_expression_t *expr)
{
	regjit_expr_list_t *curr = expr->args.body;
	jit_label_t *globalNoMatch = ctx->noMatch;
	jit_label_t didMatch = jit_label_undefined;

	jit_value_t orginalStr = ctx->str;
	while(curr != NULL)
	{
		ctx->str = jit_insn_dup(ctx->func, orginalStr);

		jit_label_t noMatch = jit_label_undefined;
		if(curr->next != NULL)
			ctx->noMatch = &noMatch;
		else
			ctx->noMatch = globalNoMatch;

		regjit_compile_expression(ctx, curr->expr);

		jit_insn_store(ctx->func, orginalStr, ctx->str);
		jit_insn_branch(ctx->func, &didMatch);

		if(ctx != NULL)
			jit_insn_label(ctx->func, &noMatch);
		curr = curr->next;
	}

	jit_insn_label(ctx->func, &didMatch);
	ctx->str = orginalStr;
}

void regjit_compile_repeat(regjit_compilation_t *ctx, regjit_expression_t *expr)
{
	regjit_repeat_t *repeat = expr->args.repeat;
	jit_label_t *globalNoMatch = ctx->noMatch;
	jit_label_t noMatch = jit_label_undefined;
	jit_label_t repeatEnd = jit_label_undefined;

	jit_value_t matchCount = jit_value_create(ctx->func, jit_type_nuint);
	jit_insn_store(ctx->func, matchCount, const(nuint, 0));

	jit_label_t loop = jit_label_undefined;
	jit_insn_label(ctx->func, &loop);

	ctx->noMatch = &noMatch;
	regjit_compile_expression(ctx, repeat->expr);

	//did match
	jit_value_t tmp = jit_insn_add(ctx->func, matchCount, const(nuint, 1));
	jit_insn_store(ctx->func, matchCount, tmp);

	if(repeat->max == SIZE_MAX)
	{
		jit_insn_branch(ctx->func, &loop);
	}
	else
	{
		tmp = jit_insn_lt(ctx->func, matchCount, const(nuint, repeat->max));
		jit_insn_branch_if(ctx->func, tmp, &loop);

		jit_insn_branch(ctx->func, &repeatEnd);
	}

	//did not match
	jit_insn_label(ctx->func, &noMatch);

	if(repeat->min != 0)
	{
		tmp = jit_insn_lt(ctx->func, matchCount, const(nuint, repeat->min));
		jit_insn_branch_if(ctx->func, tmp, globalNoMatch);
	}

	//it matched >= min and <= max times, we are done
	jit_insn_label(ctx->func, &repeatEnd);
	ctx->noMatch = globalNoMatch;
}

void regjit_compile_expression_list(regjit_compilation_t *ctx, regjit_expr_list_t *exprs)
{
	while(exprs != NULL)
	{
		regjit_compile_expression(ctx, exprs->expr);
		exprs = exprs->next;
	}
}

void regjit_compile_expression(regjit_compilation_t *ctx, regjit_expression_t *expr)
{
	switch(expr->kind)
	{
		case REGJIT_EXPR_CONST:
			regjit_compile_const(ctx, expr);
			break;

		case REGJIT_EXPR_CHARSET:
			regjit_compile_charset(ctx, expr);
			break;

		case REGJIT_EXPR_GROUP:
			regjit_compile_group(ctx, expr);
			break;

		case REGJIT_EXPR_OR:
			regjit_compile_or(ctx, expr);
			break;

		case REGJIT_EXPR_REPEAT:
			regjit_compile_repeat(ctx, expr);
			break;

		case REGJIT_EXPR_EXPRLIST:
			regjit_compile_expression_list(ctx, expr->args.body);
			break;
	}
}

void regjit_compile_global(regjit_compilation_t *ctx, regjit_expression_t *rootExpr)
{
	//TODO scan for the start of the regex in the string

	regjit_compile_expression(ctx, rootExpr);

	jit_insn_return(ctx->func, const(ubyte, 1));

	jit_insn_label(ctx->func, ctx->noMatch);
	jit_insn_return(ctx->func, const(ubyte, 0));
}

regjit_regex_t *regjit_compile(const char *expression, unsigned flags)
{
	regjit_expression_t *rootExpr = regjit_parse(expression);
	if(!rootExpr)
		return NULL;

	jit_context_t context = jit_context_create();

	jit_type_t args[] = {
		jit_type_void_ptr, // text
		jit_type_void_ptr, // textEnd
		jit_type_void_ptr, // matches
	};
	jit_type_t signature = jit_type_create_signature(jit_abi_cdecl, jit_type_ubyte, args, 3, 0);

	jit_label_t noMatch = jit_label_undefined;
	regjit_compilation_t ctx;
	ctx.func = jit_function_create(context, signature);
	ctx.str = jit_value_get_param(ctx.func, 0);
	ctx.strEnd = jit_value_get_param(ctx.func, 1);
	ctx.noMatch = &noMatch;
	ctx.groups = jit_value_get_param(ctx.func, 2);
	ctx.groupCount = 0;

	regjit_compile_global(&ctx, rootExpr);

	char *name;
	if(flags & REGJIT_FLAG_DEBUG)
	{
		name = malloc(strlen(expression) + strlen("regexp('')") + 1);
		sprintf(name, "regexp('%s')", expression);

		jit_dump_function(stdout, ctx.func, name);
		jit_function_optimize(ctx.func);
		jit_dump_function(stdout, ctx.func, name);
	}

	jit_function_compile(ctx.func);

	if(flags & REGJIT_FLAG_DEBUG)
	{
		jit_dump_function(stdout, ctx.func, name);
		free(name);
	}

	regjit_regex_t *reg = malloc(sizeof(regjit_regex_t));
	reg->func = ctx.func;
	reg->groupCount = ctx.groupCount;

	return reg;
}

void regjit_destroy(regjit_regex_t *reg)
{
	jit_context_t context = jit_function_get_context(reg->func);
	jit_context_destroy(context);
	free(reg);
}

unsigned regjit_match_count(regjit_regex_t *reg)
{
	return reg->groupCount;
}

bool regjit_match(regjit_regex_t *reg, regjit_match_t *matches, const char *text)
{
	const char *textEnd = text + strlen(text);

	uint8_t ret;
	void *args[] = {
		&text,
		&textEnd,
		&matches,
	};

	jit_function_apply(reg->func, args, &ret);
	return ret != 0;
}
