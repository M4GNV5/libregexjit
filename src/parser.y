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

regjit_expression_t *create_charset_expression(const regjit_charset_t *charset)
{
	regjit_expression_t *expr = malloc(sizeof(regjit_expression_t));
	expr->kind = REGJIT_EXPR_CHARSET;
	expr->args.charset = charset;

	return expr;
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
%token <literal> CHARSET_CUSTOM
%token <repetition> REPEAT_RANGE
%token OR
%token CHARSET_ALL
%token CHARSET_DIGITS
%token CHARSET_NON_DIGITS
%token CHARSET_WHITESPACE
%token CHARSET_NON_WHITESPACE
%token CHARSET_ALPHANUMERIC
%token CHARSET_NON_ALPHANUMERIC
%token GROUP_OPEN_NOMATCH
%token GROUP_OPEN
%token GROUP_CLOSE
%token REPEAT_ANY
%token REPEAT_AT_LEAST_ONCE
%token REPEAT_AT_MOST_ONCE
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

%right OR

%start RootRule
%parse-param {regjit_expression_t **result}
%%

RootRule:
	ExpressionList
		{
			regjit_expression_t *expr = malloc(sizeof(regjit_expression_t));
			expr->kind = REGJIT_EXPR_GROUP;
			expr->args.body = $1;

			*result = expr;
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
	  CHARSET_CUSTOM
		{
			regjit_charset_t *charset = malloc(sizeof(regjit_charset_t));
			charset->ranges = NULL;
			const char *str = $1;
			char *whitelist = (char *)$1;

			if(str[0] == '^')
			{
				charset->inverted = true;
				str++;
			}
			else
			{
				charset->inverted = false;
			}

			while(*str)
			{
				if(str[1] == '-' && str[2] != 0)
				{
					regjit_charset_range_t *range = malloc(sizeof(regjit_charset_range_t));
					range->min = str[0];
					range->max = str[2];
					range->next = charset->ranges;

					charset->ranges = range;
					str += 3;
				}
				else
				{
					*whitelist++ = *str++;
				}
			}

			if(whitelist == $1)
			{
				free(whitelist);
				charset->whitelist = NULL;
			}
			else
			{
				*whitelist++ = 0;
				charset->whitelist = $1;
			}

			$$ = malloc(sizeof(regjit_expression_t));
			$$->kind = REGJIT_EXPR_CHARSET;
			$$->args.charset = charset;
		}
	| CHARSET_ALL
		{
			$$ = create_charset_expression(&regjit_charset_all);
		}
	| CHARSET_DIGITS
		{
			$$ = create_charset_expression(&regjit_charset_digits);
		}
	| CHARSET_NON_DIGITS
		{
			$$ = create_charset_expression(&regjit_charset_non_digits);
		}
	| CHARSET_WHITESPACE
		{
			$$ = create_charset_expression(&regjit_charset_whitespace);
		}
	| CHARSET_NON_WHITESPACE
		{
			$$ = create_charset_expression(&regjit_charset_non_whitespace);
		}
	| CHARSET_ALPHANUMERIC
		{
			$$ = create_charset_expression(&regjit_charset_word);
		}
	| CHARSET_NON_ALPHANUMERIC
		{
			$$ = create_charset_expression(&regjit_charset_non_word);
		}
	;

Group:
	  GROUP_OPEN ExpressionList GROUP_CLOSE
		{
			$$ = malloc(sizeof(regjit_expression_t));
			$$->kind = REGJIT_EXPR_GROUP;
			$$->args.body = $2;
		}
	| GROUP_OPEN_NOMATCH ExpressionList GROUP_CLOSE
		{
			$$ = malloc(sizeof(regjit_expression_t));
			$$->kind = REGJIT_EXPR_EXPRLIST;
			$$->args.body = $2;
		}
	;

OrExpression:
	Expression OR Expression
		{
			regjit_expr_list_t *left = malloc(sizeof(regjit_expr_list_t));
			left->expr = $1;

			if($3->kind == REGJIT_EXPR_OR)
			{
				left->next = $3->args.body;
				$3->args.body = left;

				$$ = $3;
			}
			else
			{

				regjit_expr_list_t *right = malloc(sizeof(regjit_expr_list_t));
				right->expr = $3;
				right->next = NULL;

				left->next = right;

				$$ = malloc(sizeof(regjit_expression_t));
				$$->kind = REGJIT_EXPR_OR;
				$$->args.body = left;
			}
		}
	;

RepeatedExpression:
	Expression Repetition
		{
			regjit_expression_t *repeatExpr = malloc(sizeof(regjit_expression_t));
			repeatExpr->kind = REGJIT_EXPR_REPEAT;
			repeatExpr->args.repeat = $2;

			/* when the repeated expression is a literal in fact only the last
			   character is repeated, thus here we return an expression list
			   containing the literal without the last character and a
			   repetition of a literal only containing the last character */
			if($1->kind == REGJIT_EXPR_CONST && $1->args.literal[1] != 0)
			{
				char *literal = (char *)$1->args.literal;
				size_t len = strlen(literal);
				char last = literal[len - 1];
				literal[len - 1] = 0;

				literal = malloc(2);
				literal[0] = last;
				literal[1] = 0;

				$2->expr = malloc(sizeof(regjit_expression_t));
				$2->expr->kind = REGJIT_EXPR_CONST;
				$2->expr->args.literal = literal;

				$$ = malloc(sizeof(regjit_expression_t));
				$$->kind = REGJIT_EXPR_EXPRLIST;

				regjit_expr_list_t *curr = malloc(sizeof(regjit_expr_list_t));
				curr->expr = $1;
				curr->next = malloc(sizeof(regjit_expr_list_t));
				$$->args.body = curr;

				curr = curr->next;
				curr->expr = repeatExpr;
				curr->next = NULL;
			}
			else
			{
				$2->expr = $1;
				$$ = repeatExpr;
			}
		}
	;

Repetition:
	  REPEAT_ANY
		{
			$$ = create_repetition(0, SIZE_MAX);
		}
	| REPEAT_AT_LEAST_ONCE
		{
			$$ = create_repetition(1, SIZE_MAX);
		}
	| REPEAT_AT_MOST_ONCE
		{
			$$ = create_repetition(0, 1);
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
