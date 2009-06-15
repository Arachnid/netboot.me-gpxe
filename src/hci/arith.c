/* Recursive descent arithmetic calculator:
 * Ops: !, ~				(Highest)
 *	*, /, %
 *	+, -
 *	<<, >>
 *	<, <=, >, >=
 *	!=, ==
 *	&
 *	|
 *	^
 *	&&
 *	||				(Lowest)
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

#define SEP1			-1
#define SEP2			-1, -1

#ifndef __ARITH_TEST__
#define EPARSE		EUNIQ_01
#define EDIV0			EUNIQ_02
#define ENOOP		EUNIQ_03
#define EWRONGOP	EUNIQ_04
#else
#define EPARSE		1
#define EDIV0			2
#define ENOOP		3
#define EWRONGOP	4
#endif

char *inp, *prev, *orig;
int tok;
int err_val;
union {
	long num_value;
	char *str_value;
}tok_value;

/* Here is a table of the operators */
const char op_table[NUM_OPS * 3 + 1] = {	'!', SEP2 ,  '~', SEP2, '*', SEP2, '/', SEP2, '%', SEP2, '+', SEP2, '-', SEP2,
						'<', SEP2, '<', '=', SEP1, '<', '<', SEP1, '>', SEP2, '>', '=', SEP1, '>', '>', SEP1,  '&', SEP2,
						'|', SEP2, '^', SEP2, '&', '&', SEP1, '|', '|', SEP1, '!', '=', SEP1, '=', '=', SEP1, '\0'
};

const char *keyword_table = " \t\v()!~*/%+-<=>&|^";			/* Characters that cannot appear in a string */
signed const char op_prio[NUM_OPS]	= { 10, 10, 9, 9, 9, 8, 8, 6, 6, 7, 6, 6, 7, 4, 3, 2, 1, 0, 5, 5 };

static void ignore_whitespace ( void );
static int parse_expr ( char **buffer );

static void input ( void ) {
	char t_op[3] = { '\0', '\0', '\0'};
	char *p1, *p2;
	size_t len;
	
	if ( tok == -1 )
		return;

	prev = inp;
	ignore_whitespace();
	
	if ( *inp == '\0' ) {
		tok = -1;
		return;
	}
	
	/* Check for a number */
	if ( isdigit ( *inp ) ) {
		tok = TOK_NUMBER;
		tok_value.num_value = strtoul ( inp, &inp, 0 );
		return;
	}
	
	/* Check for a string */
	len = strcspn ( inp, keyword_table );
	
	if ( len > 0 )	{
		inp += len;
		tok = TOK_STRING;
		
		if ( ( tok_value.str_value = malloc ( len + 1 ) ) == NULL ) {
			err_val = -ENOMEM;
			return;
		}
		strncpy ( tok_value.str_value, inp - len, len );
		tok_value.str_value[len] = 0;
		return;
	}
	
	/* Check for an operator */
	t_op[0] = *inp++;
	p1 = strstr ( op_table, t_op );
	if ( !p1 ) {			/* The character is not present in the list of operators */
		tok = *t_op;
		return;
	}
	
	t_op[1] = *inp;
	p2 = strstr ( op_table, t_op );
	if ( !p2 || p1 == p2 ) {
		if ( ( p1 - op_table ) % 3 ) {			/* Without this, it would take '=' as '<=' */
			tok = *t_op;
			return;
		}
		
		tok = MIN_TOK + ( p1 - op_table ) / 3;
		return;
	}
	inp++;
	tok = MIN_TOK + ( p2 - op_table ) / 3;
}

/* Check if a string is a number: "-1" and "+42" is accepted, but not "-1a". If so, place it in num and return 1 else num = 0 and return 0 */
static int isnum ( char *string, long *num ) {
	int flag = 0;
	
	*num = 0;
	if ( *string == '+' ) {
		string++;
	} else if ( *string == '-' ) {
		flag = 1;
		string++;
	}
	*num = strtoul ( string, &string, 0 );
	if ( *string != 0 ) {
		*num = 0;
		return 0;
	}
	if ( flag )
		*num = -*num;
	return 1;
}

static void ignore_whitespace ( void ) {
	while ( isspace ( *inp ) ) {
		inp++;
	}
}

static int accept ( int ch ) {
	if ( tok == ch ) {
		input();
		return 1;
	}
	return 0;
}

static void skip ( int ch ) {
	if ( !accept ( ch ) ) {
		err_val = -EPARSE;
	}
}

static int parse_num ( char **buffer ) {
	long num = 0;
	int flag = 0;

	if ( tok == TOK_MINUS || tok == TOK_PLUS || tok == '(' || tok == TOK_NUMBER ) {
	
		if ( accept ( TOK_MINUS ) )				//Handle -NUM and +NUM
			flag = 1;
		else if ( accept ( TOK_PLUS ) ) {
		}
			
		if ( accept ( '(' ) ) {
			parse_expr ( buffer );
			if ( err_val )	{
				return ( err_val );
			}
			skip ( ')' );
			if ( err_val )	{
				free ( *buffer );
				return (err_val );
			}
			if ( flag ) {
				int t;
				t = isnum ( *buffer, &num );
				free ( *buffer );
				if ( t == 0 ) {		/* Trying to do a -string, which should not be permitted */
					return ( err_val = -EWRONGOP );
				}
				return ( ( asprintf ( buffer, "%ld", -num )  < 0 ) ? (err_val = -ENOMEM ) : 0 );
			}
			return 0;
		}
		
		if ( tok == TOK_NUMBER ) {
			if ( flag )
				num = -tok_value.num_value;
			else
				num = tok_value.num_value;
			input();
			return ( ( asprintf ( buffer, "%ld", num ) < 0) ? (err_val = -ENOMEM ) : 0 );
		}
		return ( err_val = -EPARSE );
	}
	if ( tok == TOK_STRING ) {
		*buffer = tok_value.str_value;
		input();
		return 0;
	}
	return ( err_val = -EPARSE );
}

static int eval(int op, char *op1, char *op2, char **buffer) {
	long value;
	int bothints = 1;
	long lhs, rhs;
	
	if ( op1 ) {
		if ( ! isnum ( op1, &lhs ) ) 
			bothints = 0;
	}
	
	if ( ! isnum (op2, &rhs ) )
		bothints = 0;
	
	if ( op <= 17 && ! bothints ) {
		return ( err_val = -EWRONGOP );
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
				return ( err_val = -EDIV0 );
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
			value = lhs & rhs;
			break;
		case 14: 
			value = lhs | rhs;
			break;
		case 15:
			value = lhs ^ rhs;
			break;
		case 16: 
			value = lhs && rhs;
			break;
		case 17: 
			value = lhs || rhs;
			break;
		case 18:
			value = strcmp ( op1, op2 ) != 0;
			break;
		case 19:
			value = strcmp ( op1, op2 ) == 0;
			break;
		default: 		/* This means the operator is in the op_table, but not defined in this switch statement */
			return ( err_val = -ENOOP );
	}
	return ( ( asprintf(buffer, "%ld", value)  < 0) ? ( err_val = -ENOMEM ) : 0 );
}

static int parse_prio(int prio, char **buffer) {
	int op;
	char *lc, *rc;
		
	if ( tok < MIN_TOK || tok == TOK_MINUS || tok == TOK_PLUS ) {
		parse_num ( &lc );
	} else {
		if ( tok < MIN_TOK + 2 ) {
			lc = NULL;
		} else {
			return ( err_val = -EPARSE );
		}
	}
	
	if ( err_val ) {
		return err_val;
	}
	while( tok != -1 && tok != ')' ) {
		long lhs;
		if ( tok < MIN_TOK ) {
			if ( lc )
				free(lc);
			return ( err_val = -EPARSE );
		}
		if ( op_prio[tok - MIN_TOK] <= prio - ( tok - MIN_TOK <= 1 ) ? 1 : 0 ) {
			break;
		}
		
		op  = tok;
		input();
		parse_prio ( op_prio[op - MIN_TOK], &rc );
		
		if ( err_val )	{
			if ( lc )
				free ( lc );
			return err_val;
		}
		
		lhs = eval ( op - MIN_TOK, lc, rc, buffer );
		free ( rc );
		if ( lc )
			free ( lc );
		if ( err_val ) {
			return err_val;
		}
		lc = *buffer;
 	}  
	*buffer = lc;
	return 0;
}

static int parse_expr ( char **buffer ) {
	return parse_prio ( -1, buffer );
}

int parse_arith ( char *inp_string, char **end, char **buffer ) {
	err_val = tok = 0;
	orig = inp = inp_string;
	input();
	*buffer = NULL;
	
	skip ( '(' );
	parse_expr ( buffer );
	
	if ( !err_val ) {
		
		if ( tok != ')' ) {
			free ( *buffer );
			err_val = -EPARSE;
		}
	}
	*end = inp;
	if ( err_val )	{			//Read till we get a ')'
		
		if ( tok == TOK_STRING )
			free ( tok_value.str_value );
		switch ( err_val ) {
			case -EPARSE:
				printf ( "parse error:\n%s\n", orig );
				break;
			case -EDIV0:
				printf ( "division by 0\n" );
				break;
			case -ENOOP:
				printf ( "operator undefined\n" );
				break;
			case -EWRONGOP:
				printf ( "wrong type of operand\n" );
				break;
			case -ENOMEM:
				printf("out of memory\n");
				break;
		}
		return - ( EINVAL | -err_val );
	}
	
	return 0;
}

#ifdef __ARITH_TEST__
int main ( int argc, char *argv[] ) {
	char *ret_val;
	int r = 0;
	char *head, *tail, *string, *t;
	char line[100];
	
	while ( !feof ( stdin ) ) {
		fgets ( line, 100, stdin );
		if ( line[strlen ( line ) - 1] == '\n' )
			line[strlen ( line ) - 1] = 0;
		asprintf ( &string, "%s", line );
		while ( ( head = strstr ( string, "$(" ) ) != NULL ) {
			*head++ = 0;
			r = parse_arith ( head, &tail, &ret_val );
			t = string;
			if ( r == 0 ) {
				asprintf ( &string, "%s%s%s", string, ret_val, tail );
				free ( ret_val );
			} else
				break;
			free ( t );
		}
		if ( r == 0 ) {
			printf ( "Line: %s\n", string );
		}
		free ( string );
	}
	return 0;
}
#endif
