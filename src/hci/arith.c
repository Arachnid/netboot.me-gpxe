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

#include <gpxe/parse.h>
#include <gpxe/gen_stack.h>

#define NUM_OPS 20
#define MAX_PRIO 11
#define MIN_TOK 258
#define TOK_PLUS (MIN_TOK + 5)
#define TOK_MINUS (MIN_TOK + 6)
#define TOK_NUMBER 256
#define TOK_STRING 257
#define TOK_TOTAL 20

#define SEP1 -1
#define SEP2 -1, -1

#define EPARSE (EINVAL | EUNIQ_01)
#define EDIV0 (EINVAL | EUNIQ_02)
#define ENOOP (EINVAL | EUNIQ_03)
#define EWRONGOP (EINVAL | EUNIQ_04)
#define EINCOMPL (EINVAL | EUNIQ_05)

static struct string *input_str;
static char *inp_ptr = NULL;
static int tok;
static union {
	long num_value;
	char *str_value;
}tok_value;
extern int incomplete;

/**
 * Operator table
 *
 * List of all valid operators, each consisting of 3 characters
 */
static const char op_table[NUM_OPS * 3 + 1] = { 
	'!', SEP2 ,  '~', SEP2, '*', SEP2, '/', SEP2, '%', SEP2, '+', SEP2,
	'-', SEP2, '<', SEP2, '<', '=', SEP1, '<', '<', SEP1, '>', SEP2,
	'>', '=', SEP1, '>', '>', SEP1,  '&', SEP2, '|', SEP2, '^', SEP2,
	'&', '&', SEP1, '|', '|', SEP1, '!', '=', SEP1, '=', '=', SEP1, '\0'
};

/**
 * Operator priority
 *
 * Priority of each operator
 */
static signed const char op_prio[NUM_OPS] = {
	10, 10, 9, 9, 9, 8, 8, 6, 6, 7, 6, 6, 7, 4, 3, 2, 1, 0, 5, 5
};

static STACK ( arith_stack );

static void ignore_whitespace ( void );
static int parse_expr ( void );

/**
 * Get the next token from the input
 */
static void input ( void ) {
	char t_op[3] = { '\0', '\0', '\0'};
	char *p1, *p2;
	char *strtmp = NULL;
	char *end;
	long start;
	int success;
	
	if ( tok < 0 )
		return;

	/* Whitespace between tokens is irrelevant */
	ignore_whitespace();
	
	/* Encounter end of input before closing paren => incomplete */
	if ( *inp_ptr == '\0' ) {
		incomplete = 1;
		tok = -EINCOMPL;
		return;
	}
	if ( tok == TOK_STRING )
		free ( tok_value.str_value );
	
	tok = 0;
	
	if ( inp_ptr[0] == '(' || inp_ptr[0] == ')' ) {
		tok = *inp_ptr++;
		return;
	}
	
	/* Check for an operator */
	t_op[0] = inp_ptr[0];
	p1 = strstr ( op_table, t_op );
	if ( p1 ) {
		inp_ptr++;
		t_op[1] = *inp_ptr;
		p2 = strstr ( op_table, t_op );
		if ( !p2 || p1 == p2 ) {
			if ( ( p1 - op_table ) % 3 ) {
				/* Without this, it would take '=' as '<=' */
				tok = *t_op;
				return;
			}
			tok = MIN_TOK + ( p1 - op_table ) / 3;
			return;
		}
		inp_ptr++;
		tok = MIN_TOK + ( p2 - op_table ) / 3;
		return;
	}

	start = inp_ptr - input_str->value;
	end = expand_string ( input_str, inp_ptr, arith_table, &success );
	
	/* Either incomplete or out of memory */
	if ( !end ) {
		tok = incomplete ? -EINCOMPL : -ENOMEM;
		return;
	}
	
	strtmp = strndup ( input_str->value + start,
		( end - input_str->value ) - start );
	if ( strtmp ) {
		if ( isnum ( strtmp, &tok_value.num_value ) ) {
			free ( strtmp );
			tok = TOK_NUMBER;
			DBG ( "found number: %ld\n", tok_value.num_value );
		} else {
			tok_value.str_value = strtmp;
			tok = TOK_STRING;
			DBG ( "found string: [%s]\n", tok_value.str_value );
		}
	} else
		tok = -ENOMEM;
	inp_ptr = end;
	return;
}

/**
 * Check if a string is a number
 *
 * @v string		The string to be checked
 * @v num		Pointer to store the converted value
 * @ret isnum		Whether the string is a number or not
 */
int isnum ( char *string, long *num ) {	
	*num = strtoul ( string, &string, 10 );
	if ( *string == 0 )
		return 1;
	return 0;
}

/** Skip over whitespaces */
static void ignore_whitespace ( void ) {
	while ( isspace ( *inp_ptr ) ) {
		inp_ptr++;
	}
}

/**
 * Compare the token with an expected value
 *
 * @v ch		Character to compare with
 * @ret test		Whether the token is the expected value or not
 */
static int accept ( int ch ) {
	if ( tok == ch ) {
		input();
		return 1;
	}
	return 0;
}

/**
 * Look for a token and complain if not found
 *
 * @v ch		Character to compare with
 */
static void skip ( int ch ) {
	if ( !accept ( ch ) ) {
		tok = -EPARSE;
	}
}

/**
 * Parse a number (or anything that can appear in place of a number).
 *
 * @ret err		Error encountered
 */
static int parse_num ( void ) {
	int flag = 0;
	
	/* Look for a number: [+/-]expr */
	if ( tok == TOK_MINUS || tok == TOK_PLUS
		|| tok == '(' || tok == TOK_NUMBER ) {
	
		if ( accept ( TOK_MINUS ) ) /* Look for a leading sign */
			flag = 1;
		else
			accept ( TOK_PLUS );
		
		if ( accept ( '(' ) ) {
			parse_expr ();
		
			if ( tok < 0 )	{
				return ( tok );
			}
			skip ( ')' );
			if ( tok < 0 )	{
				return ( tok );
			}
		
			if ( flag ) {	/* Had found a - sign */
				long num;
				struct string *s;
				
				s = element_at_top ( &arith_stack,
					struct string );
				
				if ( !isnum ( s->value, &num ) )
					/* Trying to do a -string */
					tok = -EWRONGOP;
				if ( tok < 0 )
					return tok;
				
				num = -num;
				free_string ( s );
				if ( asprintf ( &s->value, "%ld", num ) < 0 )
					tok = -ENOMEM;
			}
			return tok < 0 ? tok : 0;
		}
		
		if ( tok == TOK_NUMBER ) {
			struct string *s;
			
			s = stack_push ( &arith_stack, struct string );
			if ( s ) {
				if ( flag )
					tok_value.num_value =
						-tok_value.num_value;
				if ( asprintf ( &s->value, "%ld",
				 	tok_value.num_value ) < 0 )
					tok = -ENOMEM;
			}
			if ( !s || !s->value )
				tok = -ENOMEM;
			input();
			return tok < 0 ? tok : 0;
		}
		/* Error if nothing found */
		return ( tok = -EPARSE );
	}
	/* Look for a string instead */
	if ( tok == TOK_STRING ) {
		struct string *s;
		
		s = stack_push ( &arith_stack, struct string );
		if ( s ) {
			s->value = tok_value.str_value;
		} else
			tok = -ENOMEM;
		tok_value.str_value = NULL;
		input();
		return ( tok < 0 ) ? tok : 0;
	}
	/* If nothing found, it is an error */
	return ( tok = -EPARSE );
}

/**
 * Perform an operation
 *
 * @v operator		The operator
 * @v operand1		LHS of expression (NULL for unary operators)
 * @v operand2		RHS of expression
 * @ret err		Non-zero for errors
 */
int eval(int op, char *op1, char *op2, char **buffer) {
	long value;
	int bothints = 1;
	long lhs, rhs;
	*buffer = NULL;
	
	/* First, check if both are integers */
	if ( op1 ) {
		if ( ! isnum ( op1, &lhs ) )
			bothints = 0;
	}
	if ( ! isnum (op2, &rhs ) )
		bothints = 0;
	
	/* Check for the type of operands required by the operator */
	if ( op <= 17 && ! bothints ) {
		return ( tok = -EWRONGOP );
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
				return ( tok = -EDIV0 );
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
		default: 
			/* This means the operator is in the op_table,
			 * but not defined in this switch statement
			 */
			return ( tok = -ENOOP );
	}
	return ( ( asprintf(buffer, "%ld", value)  < 0)
		? ( tok = -ENOMEM ) : 0 );
}

/**
 * Priority-based parsing function
 *
 * @v priority		The priority level
 * @ret buffer		The result as a string
 * @ret err		Non-zero for errors
 */
static int parse_prio ( int prio ) {
	int op;
	char *rc;
	char *buffer;
	struct string *lc;
	
	lc = NULL;
	if ( tok < 0 ) {
		return tok;
	}
	/* Check for a non-operator or something that starts a number */
	if ( tok < MIN_TOK || tok == TOK_MINUS
		|| tok == TOK_PLUS ) {
		parse_num ( );
	} else if ( tok < MIN_TOK + 2 ) { /* A unary operator */
		lc = stack_push ( &arith_stack, struct string );
		if ( lc ) {
			lc->value = NULL;
		} else
			tok = -ENOMEM;
	} else {
			if ( tok == TOK_STRING )
				free ( tok_value.str_value );
			tok = -EPARSE;
	}
	
	/* Do until end of input or a close paren */
	while( tok >= 0 && tok != ')' ) {
		
		if ( tok < MIN_TOK )
			return ( tok = -EPARSE );
		
		/* Continue until an operator is found that:
		 * If left-associative, it has priority <= prio
		 * If right-associative, it has priority < prio
		 */
		if ( op_prio[tok - MIN_TOK] <= prio - ( tok - MIN_TOK <= 1 )
			? 1 : 0 ) {
			break;
		}
		
		op = tok;
		input();
		parse_prio ( op_prio[op - MIN_TOK] );
		
		if ( tok < 0 )	{
			return tok;
		}
		rc = element_at_top ( &arith_stack, struct string )->value;
		stack_pop ( &arith_stack );
		lc = element_at_top ( &arith_stack, struct string );
		
		/* Evaluate the expression */
		eval ( op - MIN_TOK, lc->value, rc, &buffer );
		free_string ( lc );
		free ( rc );
		lc->value = buffer;
 	}
	return tok < 0 ? tok : 0;
}

static int parse_expr ( void ) {
	return parse_prio ( -1 );
}

/**
 * Parse an arithmetic expression
 *
 * @v input		The input as a struct string
 * @v start		Pointer to start character
 * @ret end		First character after the expression
 *
 * This function expects an arithmetic expression enclosed in parens.
 * Input is passed in as a struct string, which is modified to hold the
 * evaluated expression in place of the input expression.
 */
char * parse_arith ( struct string *inp, char *orig ) {
	long start;
	char *end = NULL;
	
	arith_stack.list.next = &arith_stack.list;
	arith_stack.list.prev = &arith_stack.list;
	start = orig - inp->value;
	
	input_str = inp;
	tok = 0;
	inp_ptr = orig + 1;
	
	input();
	skip ( '(' );
	parse_expr ();
	if ( tok >= 0 ) {
		if ( tok != ')' ) {
			tok = -EPARSE;
		} else {
			struct string *s;
			
			orig = inp->value + start;
			*orig = 0;
			end = inp_ptr;
			s = element_at_top ( &arith_stack, struct string );
			orig = string3cat ( inp, s->value, end );
			end = orig + start + strlen ( s->value );
		}
	}
	free_stack_string ( &arith_stack );
	if ( tok < 0 )	{
		switch ( tok ) {
			case -EPARSE:
				printf ( "parse error\n" );
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
			case -EINCOMPL:
				break;
		}
		free_string ( inp );
		return NULL;
	}
	return end;
}
