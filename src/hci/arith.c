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

#include <lib.h>

#define NUM_OPS			20
#define MAX_PRIO		11

char *inp;
int tok;
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
			tok = *inp++;
			return;
		}
		
		t_op[0] = *inp++;
		p1 = strstr(op_table, t_op);
		if(!p1 || !*inp)
		{
			tok = *t_op;
			if(tok == ')' && brackets <= 0)
				tok = -1;
			return;
		}
		t_op[1] = *inp;
		p2 = strstr(op_table, t_op);
		if(!p2)
		{
			tok = 128 + (p1 - op_table)/3;
			return;
		}
		inp++;
		tok = 128 + (p2 - op_table)/3;
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
		printf("expected '%c', got '%c'\n", ch, *inp);
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
	
	if(accept(134))				//Handle -NUM and +NUM
		flag = -1;
	else if(accept(133)) {}
		//flag = 1;
	
	if (accept('(')) {
		brackets++;
		num = parse_expr();
		skip(')');
		brackets--;
		return flag * num;
	}
	
	while(isdigit(tok))
	{
		num = num*10 + tok - '0';
		input();
	} 
	return flag * num;
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
		
		default: printf("Undefined operator\n");
		return -1;
	}
}

static long parse_prio(int prio)
{
	long lhs = 0, rhs;
	int op;
	
	if(tok < 128 || tok == 134 || tok == 133)		//All operators are >= 128. If it is not an operator, look for number
		lhs = parse_num();
	
	while(tok != -1 && tok >= 128 && (op_prio[tok - 128] > prio + op_assoc[tok - 128]))
	{
		op  = tok;
		input();
		rhs = parse_prio(op_prio[op - 128]);
		lhs = eval(op - 128, lhs, rhs);
	}
	return lhs;
}

static long parse_expr(void)
{
	//return add_op();
	//return cmp_op();
	return parse_prio(-1);
}

long parse_arith(char *inp_string, char **end)
{
	long value;
	brackets = tok = 0;
	inp = inp_string;
	input();
	value = parse_expr();
	*end = inp;
	return value;
}
