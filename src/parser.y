%{
#include <stdio.h>
#include <stdlib.h>
#include "regexjit-internal.h"

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
	size_t size;
	regjit_expression_t *expr;
	regjit_expr_list_t *exprlist;
	regjit_repeat_t *repetition;
};

%token <literal> LITERAL
%token CHARSET_OPEN
%token CHARSET_CLOSE
%token GROUP_OPEN
%token GROUP_CLOSE
%token REPEAT_ANY
%token REPEAT_ANYPOSITIVE
%token REPEAT_OPEN
%token REPEAT_CLOSE
%token <size> NUMBER
%token COMMA

%type <expr> Expression
%type <expr> Constant
%type <expr> Charset
%type <expr> Group
%type <expr> RepeatedExpression
%type <exprlist> ExpressionList
%type <repetition> Repetition

%start ExpressionList
%%

ExpressionList:
	  Expression ExpressionList
	  	{
			yylval.exprlist = malloc(sizeof(regjit_repeat_t));
			yylval.exprlist->expr = $1;
			yylval.exprlist->next = $2;
		}
	| /* empty */
		{
			yylval.exprlist = NULL;
		}
	;

Expression:
	  Constant
	| Charset
	| Group
	| RepeatedExpression
	;

Constant:
	LITERAL
		{
			yylval.expr = malloc(sizeof(regjit_expression_t));
			yylval.expr->kind = REGJIT_EXPR_CONST;
			yylval.expr->args.literal = $1;
		}
	;

Charset:
	CHARSET_OPEN LITERAL CHARSET_CLOSE
		{
			yylval.expr = malloc(sizeof(regjit_expression_t));
			yylval.expr->kind = REGJIT_EXPR_CHARSET;
			/* TODO parse $2 */
		}
	;

Group:
	GROUP_OPEN ExpressionList GROUP_CLOSE
		{
			yylval.expr = malloc(sizeof(regjit_expression_t));
			yylval.expr->kind = REGJIT_EXPR_GROUP;
			yylval.expr->args.body = $2;
		}
	;

RepeatedExpression:
	Expression Repetition
		{
			$2->expr = $1;
			
			yylval.expr = malloc(sizeof(regjit_expression_t));
			yylval.expr->kind = REGJIT_EXPR_REPEAT;
			yylval.expr->args.repeat = $2;
		}
	;

Repetition:
	  REPEAT_ANY
		{
			yylval.repetition = create_repetition(0, SIZE_MAX);
		}
	| REPEAT_ANYPOSITIVE
		{
			yylval.repetition = create_repetition(1, SIZE_MAX);
		}
	| REPEAT_OPEN NUMBER REPEAT_CLOSE
		{
			yylval.repetition = create_repetition($2, $2);
		}
	| REPEAT_OPEN NUMBER COMMA REPEAT_CLOSE
		{
			yylval.repetition = create_repetition($2, SIZE_MAX);
		}
	| REPEAT_OPEN NUMBER COMMA NUMBER REPEAT_CLOSE
		{
			yylval.repetition = create_repetition($2, $4);
		}
	;

%%

int yyerror(char *s) {
  printf("yyerror : %s\n",s);
}

int main(void) {
  yyparse();
}