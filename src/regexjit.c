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
	jit_label_t *noMatch;

	jit_value_t groups;
	size_t groupCount;
} regjit_compilation_t;

struct regjit_regex
{
	jit_context_t jitContext;
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

void regjit_compile_group(regjit_compilation_t *ctx, regjit_expression_t *expr)
{
	jit_label_t capturingDisabled;
	size_t index = ctx->groupCount;
	ctx->groupCount++;

	capturingDisabled = jit_label_undefined;
	jit_insn_branch_if_not(ctx->func, ctx->groups, &capturingDisabled);
	jit_insn_store_relative(ctx->func, ctx->groups, 2 * index, ctx->str);
	jit_insn_label(ctx->func, &capturingDisabled);

	regjit_compile_expression_list(ctx, expr->args.body);

	capturingDisabled = jit_label_undefined;
	jit_insn_branch_if_not(ctx->func, ctx->groups, &capturingDisabled);
	jit_insn_store_relative(ctx->func, ctx->groups, 2 * index + 1, ctx->str);
	jit_insn_label(ctx->func, &capturingDisabled);
}

void regjit_compile_or(regjit_compilation_t *ctx, regjit_expression_t *expr)
{
	regjit_expr_list_t *curr = expr->args.body;
	jit_label_t *globalNoMatch = ctx->noMatch;

	while(curr != NULL)
	{
		jit_value_t orginalStr = ctx->str;
		ctx->str = jit_insn_dup(ctx->func, ctx->str);

		jit_label_t noMatch = jit_label_undefined;
		if(curr->next != NULL)
			ctx->noMatch = &noMatch;
		else
			ctx->noMatch = globalNoMatch;

		regjit_compile_expression(ctx, curr->expr);

		jit_insn_label(ctx->func, &noMatch);
		ctx->str = orginalStr;
		curr = curr->next;
	}
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
			//TODO
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
	}
}

void regjit_compile_global(regjit_compilation_t *ctx, regjit_expr_list_t *expressions)
{
	//TODO scan for the start of the regex in the string

	regjit_compile_expression_list(ctx, expressions);

	jit_insn_return(ctx->func, const(ubyte, 1));

	jit_insn_label(ctx->func, ctx->noMatch);
	jit_insn_return(ctx->func, const(ubyte, 0));

	jit_dump_function(stdout, ctx->func, "regex");
	jit_function_compile(ctx->func);
	jit_dump_function(stdout, ctx->func, "regex");
}

regjit_regex_t *regjit_compile(const char *expression, unsigned flags)
{
	regjit_expr_list_t *expressions = regjit_parse(expression);
	if(!expressions)
		return NULL;

	jit_context_t context = jit_context_create();

	jit_type_t args[] = {
		jit_type_void_ptr, // text
		jit_type_void_ptr, // matches
	};
	jit_type_t signature = jit_type_create_signature(jit_abi_cdecl, jit_type_ubyte, args, 2, 0);

	jit_label_t noMatch = jit_label_undefined;
	regjit_compilation_t ctx;
	ctx.func = jit_function_create(context, signature);
	ctx.str = jit_value_get_param(ctx.func, 0);
	ctx.noMatch = &noMatch;
	ctx.groups = jit_value_get_param(ctx.func, 1);
	ctx.groupCount = 0;

	regjit_compile_global(&ctx, expressions);

	regjit_regex_t *reg = malloc(sizeof(regjit_regex_t));
	reg->jitContext = context;
	reg->func = ctx.func;
	reg->groupCount = ctx.groupCount;

	return reg;
}

bool regjit_test(regjit_regex_t *reg, const char *text)
{
	uint8_t ret;
	char *matches = NULL;

	void *args[] = {
		&text,
		&matches,
	};

	jit_function_apply(reg->func, args, &ret);

	return ret != 0;
}

regjit_match_t *regjit_match(regjit_regex_t *reg, const char *text)
{
	uint8_t ret;
	char *matches = calloc(reg->groupCount, sizeof(regjit_match_t));

	void *args[] = {
		&text,
		&matches,
	};

	jit_function_apply(reg->func, args, &ret);

	if(ret != 0)
		return matches;

	free(matches);
	return NULL;
}
