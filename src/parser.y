%{
#include <stdio.h>
#include <stdlib.h>
#include "regexjit-internal.h"
#include "tokens.h"

regjit_repeat_t *create_repetition(size_t min, size_t max)
{
	regjit_repeat_t *repeat = malloc(sizeof(regjit_repeat_t));
	repeat->min = min;
	repeat->max = max;

	return repeat;
}
%}

%union
{
	const char *literal;
	regjit_expression_t *expr;
	regjit_expr_list_t *exprlist;
	regjit_repeat_t *repetition;
};

%define parse.error verbose

%token <literal> LITERAL
%token <repetition> REPEAT_RANGE
%token OR
%token CHARSET_OPEN
%token CHARSET_CLOSE
%token GROUP_OPEN
%token GROUP_CLOSE
%token REPEAT_ANY
%token REPEAT_ANYPOSITIVE
%token REPEAT_OPEN
%token REPEAT_CLOSE
%token COMMA

%type <expr> Expression
%type <expr> Constant
%type <expr> Charset
%type <expr> Group
%type <expr> OrExpression
%type <expr> RepeatedExpression
%type <exprlist> ExpressionList
%type <repetition> Repetition

%start RootRule
%parse-param {regjit_expr_list_t **result}
%%

RootRule:
	ExpressionList
		{
			*result = $1;
		}
	;

ExpressionList:
	  Expression ExpressionList
	  	{
			$$ = malloc(sizeof(regjit_expr_list_t));
			$$->expr = $1;
			$$->next = $2;
		}
	| /* empty */
		{
			$$ = NULL;
		}
	;

Expression:
	  Constant
	| Charset
	| Group
	| OrExpression
	| RepeatedExpression
	;

Constant:
	LITERAL
		{
			$$ = malloc(sizeof(regjit_expression_t));
			$$->kind = REGJIT_EXPR_CONST;
			$$->args.literal = $1;
		}
	;

Charset:
	CHARSET_OPEN LITERAL CHARSET_CLOSE
		{
			$$ = malloc(sizeof(regjit_expression_t));
			$$->kind = REGJIT_EXPR_CHARSET;
			/* TODO parse $2 */
		}
	;

Group:
	GROUP_OPEN ExpressionList GROUP_CLOSE
		{
			$$ = malloc(sizeof(regjit_expression_t));
			$$->kind = REGJIT_EXPR_GROUP;
			$$->args.body = $2;
		}
	;

OrExpression:
	Expression OR Expression
		{
			regjit_expr_list_t *right = malloc(sizeof(regjit_expr_list_t));
			right->expr = $3;
			right->next = NULL;

			regjit_expr_list_t *left = malloc(sizeof(regjit_expr_list_t));
			left->expr = $1;
			left->next = right;

			$$ = malloc(sizeof(regjit_expression_t));
			$$->kind = REGJIT_EXPR_OR;
			$$->args.body = left;
		}
	;

RepeatedExpression:
	Expression Repetition
		{
			$2->expr = $1;

			$$ = malloc(sizeof(regjit_expression_t));
			$$->kind = REGJIT_EXPR_REPEAT;
			$$->args.repeat = $2;
		}
	;

Repetition:
	  REPEAT_ANY
		{
			$$ = create_repetition(0, SIZE_MAX);
		}
	| REPEAT_ANYPOSITIVE
		{
			$$ = create_repetition(1, SIZE_MAX);
		}
	| REPEAT_RANGE
		{
			$$ = $1;
		}
	;

%%

int yyerror(char *err) {
	puts(err);
}

/*int main(void) {
	yyparse();
}*/
