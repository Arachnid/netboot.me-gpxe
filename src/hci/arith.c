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
#define TOK_TOTAL		20

char *inp, *prev;
int tok;
int err_val;		//skip, parse_num, eval
long tok_value;

char *op_table = "!@@" "~@@" "*@@" "/@@" "%@@" "+@@" "-@@" "<@@" "<=@" "<<@" ">@@" ">=@" ">>@" "!=@" "==@" "&@@" "|@@" "^@@" "&&@" "||@";
char *keyword_table = " \t\v()!~*/%+-<=>&|^";			//Characters that cannot appear in a string
signed char op_prio[NUM_OPS]	= { 10, 10, 9, 9, 9, 8, 8, 6, 6, 7, 6, 6, 7, 5, 5, 4, 3, 2, 1, 0 };

/*
	Changes:
	1. Common function for all operators.
	2. Common input function.
	
	Notes:
	1. Better way to store operators of > 1 characters? I have tried to keep handling operators consistent.
*/	
	
static void ignore_whitespace(void);
//static long get_value(char *var, int size);

static void input()
{
	char t_op[3] = { '\0', '\0', '\0'};
	char *p1, *p2;
	size_t len;
	
	if(tok == -1)
		return;

	prev = inp;
	ignore_whitespace();
	
	if(*inp != '\0')
	{
		if(isdigit(*inp))
		{
			tok_value = 0;
			tok = TOK_NUMBER;
			tok_value = strtoul(inp, &inp, 0);
			return;
		}
		
		len = strcspn(inp, keyword_table);
		
		if(len > 0)
		{
			char str_val[len + 1];
			strncpy(str_val, inp, len);
			str_val[len] = '\0';
			asprintf((char **)&tok_value, "%s", str_val);
			inp += len;
			tok = TOK_STRING;
			return;
		}
		
		t_op[0] = *inp++;
		p1 = strstr(op_table, t_op);
		if(!p1)
		{
			tok = *t_op;
			return;
		}
		t_op[1] = *inp;
		p2 = strstr(op_table, t_op);
		if(!p2 || p1 == p2)
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

static void skip(int ch)
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

//"!" "~" "*" "/" "%" "+" "-" "<" "<=" "<<" ">" ">=" ">>" "!=" "==" "&" "|" "^" "&&" "||";
static int eval(int op, char *op1, char *op2, char **buffer)
{
	long value;
	
	long lhs, rhs;
	int flag1 = 0, flag2 = 0;
	
	if(*op1 == '-')
	{
		flag1 = 1;
		op1++;
	}
	if(*op2 == '-')
	{
		flag2 = 1;
		op2++;
	}
	lhs = strtoul(op1, NULL, 0);
	if(flag1) lhs = -lhs;
	rhs = strtoul(op2, NULL, 0);
	if(flag2) rhs = -rhs;
	
	switch(op)
	{
		case 0:
			value = !rhs;
			break;
		case 1: 
			value = ~rhs;
			break;
		case 2: 
			value = lhs * rhs;
			break;
		case 3: 
			value = lhs / rhs;
			break;
		case 4: 
			value = lhs % rhs;
			break;
		case 5:
			value = lhs + rhs;
			break;
		case 6: 
			value = lhs - rhs;
			break;
		case 7: 
			value = lhs < rhs;
			break;
		case 8: 
			value = lhs <= rhs;
			break;
		case 9: 
			value = lhs << rhs;
			break;
		case 10: 
			value = lhs > rhs;
			break;
		case 11: 
			value = lhs >= rhs;
			break;
		case 12: 
			value = lhs >> rhs;
			break;
		case 13:
			value = strcmp(op1, op2) ? 1 : 0;
			break;
		case 14:
			value = !strcmp(op1, op2);
			break;
		case 15:
			value = lhs & rhs;
			break;
		case 16: 
			value = lhs | rhs;
			break;
		case 17:
			value = lhs ^ rhs;
			break;
		case 18: 
			value = lhs && rhs;
			break;
		case 19: 
			value = lhs || rhs;
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
	else asprintf(&lc, " ");
	if(err_val)
		return 0;
	while(tok != -1 && tok >= MIN_TOK && (op_prio[tok - MIN_TOK] > prio - (tok - MIN_TOK <= 1) ? 1 : 0) ) 
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
	char *tail;
	r = parse_arith(argv[1], &tail, &ret_val);
	if(r < 0)
		printf("%d Tail: %s\n", r, tail);
	else
		printf("%s Tail:%s\n", ret_val, tail);
	return 0;
}
#endif
