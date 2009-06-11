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

#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

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
int brackets;

char *op_table = "!@@" "~@@" "*@@" "/@@" "%@@" "+@@" "-@@" "=@@" "<@@" "<=@" "<<@" ">@@" ">=@" ">>@" "!=@"  "&@@" "|@@" "^@@" "&&@" "||@";
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

static void input(void) {
	char t_op[3] = { '\0', '\0', '\0'};
	char *p1, *p2;
	size_t len;
	
	if(tok == -1)
		return;

	prev = inp;
	ignore_whitespace();
	
	if(*inp != '\0') {
		if(isdigit(*inp)) {
			tok_value = 0;
			tok = TOK_NUMBER;
			tok_value = strtoul(inp, &inp, 0);
			return;
		}
		
		len = strcspn(inp, keyword_table);
		
		if(len > 0)	{
			char str_val[len + 1];
			strncpy(str_val, inp, len);
			str_val[len] = '\0';
			if(asprintf((char **)&tok_value, "%s", str_val) < 0) {
				err_val = -ENOMEM;
			}
			inp += len;
			tok = TOK_STRING;
			return;
		}
		
		t_op[0] = *inp++;
		p1 = strstr(op_table, t_op);
		if(!p1) {
			tok = *t_op;
			return;
		}
		t_op[1] = *inp;
		p2 = strstr(op_table, t_op);
		if(!p2 || p1 == p2) {
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

static void ignore_whitespace(void) {
	while (isspace(*inp)) {
		inp++;
	}
}

static int accept(int ch) {
	if (tok == ch) {
		input();
		return 1;
	}
	return 0;
}

static void skip(int ch) {
	if (!accept(ch)) {
		err_val = -1;
		printf("expected '%c', got '%c'\n", (char)ch, (char)tok);
	}
}

static int parse_num(char **buffer) {
	long num = 0;
	int flag = 1;
	
	if(tok == TOK_MINUS || tok == TOK_PLUS || tok == '(' || tok == TOK_NUMBER) {
	
		if(accept(TOK_MINUS))				//Handle -NUM and +NUM
			flag = -1;
		else if(accept(TOK_PLUS)) {}
	
		if (accept('(')) {
			brackets++;
			parse_expr(buffer);
			if(err_val)	{
				return -1;
			}
			skip(')');
			brackets--;
			if(err_val)	{
				free(*buffer);
				return -1;
			}
			if(flag < 0) {
				if(**buffer == '-') {
					**buffer = '+';
				} else {
					char t[strlen(*buffer) + 2];
					t[0] = '-';
					strcpy(t + 1, *buffer);
					free(*buffer);
					if(asprintf(buffer, "%s", t) < 0) {
						err_val = -ENOMEM;
						return -ENOMEM;
					}
				}				
			}
			return strlen(*buffer);
		}
		if(tok == TOK_NUMBER) {
			num = flag * tok_value;
			input();
			if(asprintf(buffer, "%ld", num) < 0) {
				err_val = -ENOMEM;
				return err_val;
			}
			return strlen(*buffer);
		}
		err_val = -1;
		return -1;
	}
	
	if (tok == TOK_STRING)	{
		*buffer = (char *)tok_value;
		input();
		return strlen(*buffer);
	}
	err_val = -1;
	return -1;
}

//"!" "~" "*" "/" "%" "+" "-" "<" "<=" "<<" ">" ">=" ">>" "!=" "==" "&" "|" "^" "&&" "||";

static int eval(int op, char *op1, char *op2, char **buffer) {
	long value;
	
	long lhs, rhs;
	int flag1 = 0, flag2 = 0;
	char *o1 = op1, *o2 = op2;
	
	if(op1 && *op1 == '-') {
		flag1 = 1;
		o1++;
	}
	if(*op2 == '-') {
		flag2 = 1;
		o2++;
	}
	lhs = op1 ? strtoul(o1, NULL, 0) : 0;
	if(flag1) {
		lhs = -lhs;
	}
	rhs = strtoul(o2, NULL, 0);
	if(flag2) {
		rhs = -rhs;
	}
	
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
			if(rhs != 0)
				value = lhs / rhs;
			else {
				err_val = -2;
			}
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
			value = !strcmp(op1, op2);
			break;
		case 8: 
			value = lhs < rhs;
			break;
		case 9: 
			value = lhs <= rhs;
			break;
		case 10: 
			value = lhs << rhs;
			break;
		case 11: 
			value = lhs > rhs;
			break;
		case 12: 
			value = lhs >= rhs;
			break;
		case 13: 
			value = lhs >> rhs;
			break;
		case 14:
			value = strcmp(op1, op2) ? 1 : 0;
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
		
		default: 		//This should not happen
			*buffer = NULL;
			err_val = -3;
			return err_val;	
	}
	if(asprintf(buffer, "%ld", value) < 0) {
		err_val = -ENOMEM;
		return err_val;
	}
	return strlen(*buffer);
}

static int parse_prio(int prio, char **buffer) {
	int op;
	char *lc, *rc;
		
	if(tok < MIN_TOK || tok == TOK_MINUS || tok == TOK_PLUS) {
		parse_num(&lc);
	} else {
		if(tok < MIN_TOK + 2) {
			lc = NULL;
		} else {
			err_val = -1;
			return -1;
		}
	}
	
	if(err_val) {
		return -1;
	}
	while(tok != -1 && tok != ')') {
		long lhs;
		if(tok < MIN_TOK) {
			err_val = -1;
			if(lc) free(lc);
			return -1;
		}
		if(op_prio[tok - MIN_TOK] <= prio - (tok - MIN_TOK <= 1) ? 1 : 0) {
			break;
		}
		
		op  = tok;
		input();
		parse_prio(op_prio[op - MIN_TOK], &rc);
		
		if(err_val)	{
			if(lc) free(lc);
			return -1;
		}
		
		lhs = eval(op - MIN_TOK, lc, rc, buffer);
		free(rc);
		if(lc) free(lc);
		if(err_val) {
			return -1;
		}
		lc = *buffer;
	}
	*buffer = lc;
	return strlen(*buffer);
}

static int parse_expr(char **buffer) {
	return parse_prio(-1, buffer);
}

int parse_arith(char *inp_string, char **end, char **buffer) {
	err_val = tok = 0;
	inp = inp_string;
	brackets = 0;
	input();
	*buffer = NULL;
	
	skip('(');
	brackets++;
	parse_expr(buffer);
	if(!err_val) {
		skip(')');
	}
	
	if(err_val)	{			//Read till we get a ')'
		*end = strchr(inp, ')');
		if(!*end) {
			*end = inp;
		} else {
			(*end) += 1;
		}
		switch (err_val) {
			case -1:
				printf("parse error\n");
				break;
			case -2:
				printf("division by 0\n");
				break;
			case -ENOMEM:
				printf("out of memory\n");
				break;
		}
		return -1;
	}
	
	*end = prev;
	return strlen(*buffer);
}

#ifdef __ARITH_TEST__
int main(int argc, char *argv[]) {
	char *ret_val;
	int r;
	char *tail;
	r = parse_arith(argv[1], &tail, &ret_val);
	if(r < 0)
		printf("%d  %s Tail: %s\n", r, ret_val, tail);
	else
		printf("%s Tail:%s\n", ret_val, tail);
	return 0;
}
#endif
