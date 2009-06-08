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
#define MIN_TOK		258
#define TOK_PLUS		(MIN_TOK + 5)
#define TOK_MINUS	(MIN_TOK + 6)
#define TOK_NUMBER	256
#define TOK_STRING	257

char *inp, *prev;
int tok;
int err_val;		//skip, parse_num, eval
long tok_value;

char *op_table = "!@@" "~@@" "*@@" "/@@" "%@@" "+@@" "-@@" "<@@" "<=@" "<<@" ">@@" ">=@" ">>@" "!=@" "==@" "&@@" "|@@" "^@@" "&&@" "||@";
char *keyword_table = " \t()";
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
#ifndef __ARITH_TEST__
static long strtol(char *string, char **eptr, int base);
#endif
//static long get_value(char *var, int size);

static void input()
{
	char t_op[3] = { 0, 0, 0};
	char *p1, *p2;
	
	if(tok == -1)
		return;

	prev = inp;
	ignore_whitespace();
	
	if(*inp)
	{
		if(isdigit(*inp))
		{
			tok_value = 0;
			tok = TOK_NUMBER;
			while(isdigit(*inp))
			{
				tok_value = strtol(inp, &inp, 0);
			}
			return;
		}
				
		t_op[0] = *inp++;
				
		p1 = strstr(op_table, t_op);
		if(!p1)					//Ok: so this is not a number, and not an operator
		{
			char *start = inp - 1;
			if(strchr(keyword_table, *t_op) || (*t_op == '$' && *inp == '(') )
			{
				tok = *t_op;
				return;
			}
			
			while(*inp && !strchr(keyword_table, *inp) && !strchr(op_table, *inp))
				inp++;
			{
				char str_val[inp - start + 1];
				char *s;
				strncpy(str_val, start, inp - start);
				str_val[inp - start] = 0;
				tok = TOK_STRING;
				asprintf(&s, "%s", str_val);
				tok_value = (long)s;
				return;
			}
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

static int parse_expr(char **buffer);

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

static int parse_num(char **buffer)
{
	long num = 0;
	int flag = 1;
	
	if(accept(TOK_MINUS))				//Handle -NUM and +NUM
		flag = -1;
	else if(accept(TOK_PLUS)) {}
	
	if (accept('(')) {
		parse_expr(buffer);
		skip(')');
		if(flag < 0)
		{
			if(**buffer == '-')
				**buffer = '+';
			else
			{
				char t[strlen(*buffer) + 2];
				t[0] = '-';
				strcpy(t + 1, *buffer);
				free(*buffer);
				return asprintf(buffer, "%s", t);
			}				
		}
		return strlen(*buffer);
	}
	
	if(tok == TOK_NUMBER)
	{
		num = flag * tok_value;
		input();
		return asprintf(buffer, "%ld", num);
	} else if (tok == TOK_STRING)
	{
		*buffer = (char *)tok_value;
		input();
		return strlen(*buffer);
	}
		err_val = -1;
	return 0;
}

#ifndef __ARITH_TEST__
static long strtol(char *string, char **eptr, int base)		//Just a temporary measure. It seems strtol is not defined
{
	int flag = 1;
	long value = 0;
	
	if(base != 0 && base != 10) return 0;
	
	while(isspace(*string)) string++;
	if(*string == '-') flag = -1;
	else if(*string == '+') flag = 1;
	while(*string && isdigit(*string))
	{
		value = value * 10 + (*string++ - '0');
	}
	if(eptr)
		*eptr = string;
	return value;
}
#endif

//"!" "~" "*" "/" "%" "+" "-" "<" "<=" "<<" ">" ">=" ">>" "!=" "==" "&" "|" "^" "&&" "||";
static int eval(int op, char *op1, char *op2, char **buffer)
{
	long value;
	switch(op)
	{
		case 0:
			value = !strtol(op2, NULL, 0);
			break;
		case 1: 
			value = ~strtol(op2, NULL, 0);
			break;
		case 2: 
			value = strtol(op1, NULL, 0) * strtol(op2, NULL, 0);
			break;
		case 3: 
			value = strtol(op1, NULL, 0) / strtol(op2, NULL, 0);
			break;
		case 4: 
			value = strtol(op1, NULL, 0) % strtol(op2, NULL, 0);
			break;
		case 5:
			value = strtol(op1, NULL, 0) + strtol(op2, NULL, 0);
			break;
		case 6: 
			value = strtol(op1, NULL, 0) - strtol(op2, NULL, 0);
			break;
		case 7: 
			value = strtol(op1, NULL, 0) < strtol(op2, NULL, 0);
			break;
		case 8: 
			value = strtol(op1, NULL, 0) <= strtol(op2, NULL, 0);
			break;
		case 9: 
			value = strtol(op1, NULL, 0) << strtol(op2, NULL, 0);
			break;
		case 10: 
			value = strtol(op1, NULL, 0) > strtol(op2, NULL, 0);
			break;
		case 11: 
			value = strtol(op1, NULL, 0) >= strtol(op2, NULL, 0);
			break;
		case 12: 
			value = strtol(op1, NULL, 0) >> strtol(op2, NULL, 0);
			break;
		case 13:
			value = strtol(op1, NULL, 0) != strtol(op2, NULL, 0);
			break;
		case 14:
			value = !strcmp(op1, op2);
			break;
		case 15:
			value = strcmp(op1, op2) ? 1 : 0;
			break;
		case 16: 
			value = strtol(op1, NULL, 0) | strtol(op2, NULL, 0);
			break;
		case 17:
			value = strtol(op1, NULL, 0) ^ strtol(op2, NULL, 0);
			break;
		case 18: 
			value = strtol(op1, NULL, 0) && strtol(op2, NULL, 0);
			break;
		case 19: 
			value = strtol(op1, NULL, 0) || strtol(op2, NULL, 0);
			break;
		
		default: 
			err_val = -1;
			return 0;
			//printf("Undefined operator\n");
	}
	return asprintf(buffer, "%ld", value);
}

static int parse_prio(int prio, char **buffer)
{
	int op;
	char *lc, *rc;
	
	if(tok < MIN_TOK || tok == TOK_MINUS || tok == TOK_PLUS)		//All operators are >= 128. If it is not an operator, look for number
	{
		parse_num(&lc);
	}
	if(err_val)
		return 0;
	while(tok != -1 && tok >= MIN_TOK && (op_prio[tok - MIN_TOK] > prio + op_assoc[tok - MIN_TOK]))
	{
		long lhs;
		op  = tok;
		input();
		parse_prio(op_prio[op - MIN_TOK], &rc);
		
		if(err_val)
			return 0;
		
		lhs = eval(op - MIN_TOK, lc, rc, buffer);
		free(rc);
		free(lc);
		lc = *buffer;
		
		if(err_val)
			return 0;
	}
	*buffer = lc;
	return strlen(*buffer);
}

static int parse_expr(char **buffer)
{
	return parse_prio(-1, buffer);
}

int parse_arith(char *inp_string, char **end, char **buffer)
{
	err_val = tok = 0;
	inp = inp_string;
	input();
	
	parse_expr(buffer);
	
	if(err_val)				//Read till we get a ')'
	{
		*end = strchr(inp, ')');
		if(!*end)
			*end = inp;
		else	end++;
	}
	else
	*end = prev;
	
	if(err_val)
	{
		printf("parse error\n");
		return 0;
	}
	
	return strlen(*buffer);
	
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
