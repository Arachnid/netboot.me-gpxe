/*
 * Recursive descent arithmetic calculator:
 *   + - * / ( )
 */

/*
Ops: !, ~				(Highest)
	*, /, %
	+, -
	<<, >>
	<, <=, >, >=
	!=, ==
	&
	|
	^
	&&
	||				(Lowest)
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifndef __ARITH_TEST__
#include <lib.h>
#endif

#define NUM_OPS		20
#define MAX_PRIO		11
#define MIN_TOK		257
#define TOK_PLUS		(MIN_TOK + 5)
#define TOK_MINUS	(MIN_TOK + 6)
#define TOK_NUMBER	256

char *inp;
int tok;
int err_val;
long tok_value;
int brackets;

char *op_table = "!@@" "~@@" "*@@" "/@@" "%@@" "+@@" "-@@" "<@@" "<=@" "<<@" ">@@" ">=@" ">>@" "!=@" "==@" "&@@" "|@@" "^@@" "&&@" "||@";
signed char op_prio[NUM_OPS]	= { 10, 10, 9, 9, 9, 8, 8, 6, 6, 7, 6, 6, 7, 5, 5, 4, 3, 2, 1, 0 };
signed char op_assoc[NUM_OPS] = { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
char op_arity[NUM_OPS] = { 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
	
/*
	Changes:
	1. Common function for all operators.
	2. Common input function.
	
	Notes:
	1. Better way to store operators of > 1 characters? I have tried to keep handling operators consistent.
	2. Currently, only ! and ~ are unary and right-associative. They are also the highest precedence. If they remain so, we can get rid of op_assoc and op_arity.
*/	
	
static void ignore_whitespace(void);
//static long get_value(char *var, int size);

#if 0
struct variable
{
	char *name;
	long value;
	struct variable *next;
};

struct variable *sym_table;
#endif

static void input()
{
	char t_op[3] = { 0, 0, 0};
	char *p1, *p2;
	
	if(tok == -1)
		return;

	ignore_whitespace();
	
	if(*inp)
	{
		if(isdigit(*inp))
		{
			tok_value = 0;
			tok = TOK_NUMBER;
			while(isdigit(*inp))
			{
				tok_value = tok_value*10 + *inp++ - '0';
			}
			return;
		}
		
		
		t_op[0] = *inp++;
		
		if(*t_op == ')')
		{
			brackets--;
			tok = ')';
			return;
		}
		
		p1 = strstr(op_table, t_op);
		if(!p1 || !*inp)
		{
			tok = *t_op;
			return;
		}
		t_op[1] = *inp;
		p2 = strstr(op_table, t_op);
		if(!p2)
		{
			tok = MIN_TOK + (p1 - op_table)/3;
			return;
		}
		inp++;
		tok = MIN_TOK + (p2 - op_table)/3;
	}
	else
		tok = -1;	
}

static long parse_expr(void);

static void ignore_whitespace(void)
{
	while (isspace(*inp)) {
		inp++;
	}
}

static int accept(int ch)
{
	if (tok == ch) {
		input();
		return 1;
	}
	return 0;
}

static void skip(char ch)
{
	if (!accept(ch)) {
		err_val = -1;
		printf("expected '%c', got '%c'\n", ch, tok);
	}
}

#if 0
static long parse_var(void)
{
	int len = 0;
	char *ptr;
	struct variable *cur = sym_table;
	ptr = inp;
	while(isalphanum(tok) || tok == '_')
	{
		input();
		len++;
	}
	while(cur)
	{
		if(!strncmp(cur -> name, ptr, len) && cur -> name[len+1] == 0)
			return cur -> value;
		cur = cur -> next;
	}
	printf("Variable not found\n");
	return 0;
}
#endif

static long parse_num(void)
{
	long num = 0;
	int flag = 1;
	
	if(accept(TOK_MINUS))				//Handle -NUM and +NUM
		flag = -1;
	else if(accept(TOK_PLUS)) {}
	
	if (accept('(')) {
	brackets++;
		num = parse_expr();
		skip(')');
		return flag * num;
	}
	
	if(tok == TOK_NUMBER)
	{
		num = flag * tok_value;
		input();
		return num;
	}
	else
		err_val = -1;
	return 0;
}

#if 0
static long mul_op(void)
{
	long a = parse_num();
	/* This is left-associative */
	for (;;) {
		if (accept('*')) {
			a *= parse_num();
		} else if (accept('/')) {
			a /= parse_num();
		} else {
			return a;
		}
	}
}

static long add_op(void)
{
	long a = mul_op();
	/* Right-associative is easy */
	if (accept('+')) {
		return a + parse_expr();
	} else if (accept('-')) {
		return a - parse_expr();
	} else {
		return a;
	}
}

static long cmp_op(void)
{
	long a = add_op();
	
	if(accept(133)) {
		return a >= parse_expr();
	}
	else return a;
} 
#endif

//"!" "~" "*" "/" "%" "+" "-" "<" "<=" "<<" ">" ">=" ">>" "!=" "==" "&" "|" "^" "&&" "||";
static long eval(int op, long op1, long op2)
{
	switch(op)
	{
		case 0: return !op2;
		case 1: return ~op2;
		case 2: return op1 * op2;
		case 3: return op1 / op2;
		case 4: return op1 % op2;
		case 5: return op1 + op2;
		case 6: return op1 - op2;
		case 7: return op1 < op2;
		case 8: return op1 <= op2;
		case 9: return op1 << op2;
		case 10: return op1 > op2;
		case 11: return op1 >= op2;
		case 12: return op1 >> op2;
		case 13: return op1 != op2;
		case 14: return op1 == op2;
		case 15: return op1 & op2;
		case 16: return op1 | op2;
		case 17: return op1 ^ op2;
		case 18: return op1 && op2;
		case 19: return op1 || op2;
		
		default: 
			err_val = -1;
			printf("Undefined operator\n");
		return -1;
	}
}

static long parse_prio(int prio)
{
	long lhs = 0, rhs;
	int op;
	
	//input();
	
	if(tok < MIN_TOK || tok == TOK_MINUS || tok == TOK_PLUS)		//All operators are >= 128. If it is not an operator, look for number
	{
		lhs = parse_num();
	}
	if(err_val)
		return 0;
	while(tok != -1 && tok >= MIN_TOK && (op_prio[tok - MIN_TOK] > prio + op_assoc[tok - MIN_TOK]))
	{
		op  = tok;
		input();
		rhs = parse_prio(op_prio[op - MIN_TOK]);
		
		if(err_val)
			return 0;
		
		lhs = eval(op - MIN_TOK, lhs, rhs);
	}
	return lhs;
}

static long parse_expr(void)
{
	return parse_prio(-1);
}

int parse_arith(char *inp_string, char **end, char **buffer)
{
	long value;
	brackets = err_val = tok = 0;
	inp = inp_string;
	input();
	value = parse_expr();
	
	if(err_val)				//Read till we get a ')'
	{
		while(*inp && *inp != ')')
			inp++;
		input();
	}
	
	*end = inp;
	skip(')');
	
	if(err_val)
	{
		printf("Parse error\n");
		return -1;
	}
	
	if(buffer)
		return asprintf(buffer, "%ld", value);
	return -1;
}

#ifdef __ARITH_TEST__
int main(int argc, char *argv[])
{
	char *ret_val;
	int r;
	int brackets = 0;
	char *tail;
	r = parse_arith(argv[1], &tail, &ret_val);
	if(r < 0)
		printf("%d Tail: %s\n", r, tail);
	else
		printf("%s Tail:%s\n", ret_val, tail);
	return 0;
}
#endif
