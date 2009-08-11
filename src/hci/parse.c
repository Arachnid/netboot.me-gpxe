#include <gpxe/parse.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gpxe/settings.h>

/* Just to mark the current line as incomplete */
int incomplete;
static char * dollar_expand ( struct string *s, char *inp, int *success );
static char * quotes_expand ( struct string *string, char *start,
	int *success );
static void delchar ( char *charp, int num_del );

#define CHAR_TABLE_END { .token = 0, .parse_func = NULL }

/* Table for arithmetic expressions */
const struct char_table arith_table[23] = {
	{
		.token = '\\',
		.parse_func = parse_escape
	},
	{
		.token = '"',
		.parse_func = quotes_expand
	},
	{
		.token = '$',
		.parse_func = dollar_expand
	},
	{
		.token = '\'',
		.parse_func = quotes_expand
	},
	{
		.token = ' ',
	},
	{
		.token = '\t',
	},
	{
		.token = '\n',
	},
	{
		.token = '~',
	},
	{
		.token = '!',
	},
	{
		.token = '*',
	},
	{
		.token = '/',
	},
	{
		.token = '%',
	},
	{
		.token = '+',
	},
	{
		.token = '-',
	},
	{
		.token = '<',
	},
	{
		.token = '=',
	},
	{
		.token = '>',
	},
	{
		.token = '&',
	},
	{
		.token = '|',
	},
	{
		.token = '^',
	},
	{
		.token = '(',
	},
	{
		.token = ')',
	},
	CHAR_TABLE_END,
};

/** Table for parsing text in double-quotes */
const struct char_table dquote_table[4] = {
	{
		.token = '"',
	},
	{
		.token = '$',
		.parse_func = dollar_expand
	},
	{
		.token = '\\',
		.parse_func = parse_escape
	},
	CHAR_TABLE_END
};

/** Table to parse text in single-quotes */
const struct char_table squote_table[2] = {
	{
		.token = '\'',
	},
	CHAR_TABLE_END
};

/** Table to start with */
const struct char_table toplevel_table[7] = {
	{
		.token = '\\',
		.parse_func = parse_escape
	},
	{
		.token = '"',
		.parse_func = quotes_expand
	},
	{
		.token = '$',
		.parse_func = dollar_expand
	},
	{
		.token = '\'',
		.parse_func = quotes_expand
	},
	{
		.token = ' ',
	},
	{
		.token = '\t',
	},
	CHAR_TABLE_END
};

/** Table to parse text after a $ */
const struct char_table dollar_table[3] = {
	{
		.token = '}',
	},
	{
		.token = '$',
		.parse_func = dollar_expand
	},
	CHAR_TABLE_END
};

/**
 * Combine a struct string with 2 char *
 *
 * @v string	Pointer to the struct string that is to be modified
 * @v s2	First char *
 * @v s3	Second char *
 * @ret	string	Return the modified string
 *
 * The resultant string: string.s2.s3 (. represents concatenation) is
 * malloc'd and a pointer (char *) is returned.
 */
char * string3cat ( struct string *s1, const char *s2,
	const char *s3 ) {
		
	char *tmp = s1->value;
		
	if ( s1->value )
		asprintf ( &s1->value, "%s%s%s", s1->value, s2, s3 );
	else {
		stringcpy ( s1, s2 );
		if ( s1->value )
			stringcat ( s1, s3 );
	}
	free ( tmp );
	return s1->value;
}

/**
 * Function to copy a char * into a struct string
 *
 * @v string	struct string to copy into
 * @v s		char * to copy from
 *
 * Analogous to a strcpy. A copy of s is made on the heap, and string
 * will contain a pointer to it.
 */
char * stringcpy ( struct string *s1, const char *s2 ) {
	char *tmp = s1->value;
	s1->value = strdup ( s2 );
	free ( tmp );
	return s1->value;
}

/**
 * Function to concatenate a struct string and a char *
 *
 * @v string	struct string holds the first part, and will hold the result
 * @v s		char * to concatenate
 *
 * Analogous to strcat.
 */
char * stringcat ( struct string *s1, const char *s2 ) {
	char *tmp;
	if ( !s1->value )
		return stringcpy ( s1, s2 );
	if ( !s2 )
		return s1->value;
	tmp = s1->value;
	asprintf ( &s1->value, "%s%s", s1->value, s2 );
	free ( tmp );
	return s1->value;
}

/**
 * Free a struct string
 *
 * @v string	struct string to be freed
 *
 * struct string contains a pointer to a char array on the heap. It needs
 * to be freed after use.
 */
void free_string ( struct string *s ) {
	free ( s->value );
	s->value = NULL;
}

/**
 * Expand the text following a $
 *
 * @v string	Text to be expanded, as a struct string
 * @v start	Points to the $
 * @ret end	Pointer to character after the $ expansion
 *
 * This function takes in input as a struct string, and looks for a variable
 * expansion. Note that recursive expansion is permitted. If the input is
 * abc$<exp>xyz, at the end, string will hold abc<expanded>xyz.
 */
char * dollar_expand ( struct string *string, char *start, int *success ) {
	int len;
	char *end;
	
	len = start - string->value;

	if ( start[1] == '{' ) {
		int s2;
		end = expand_string ( string, start + 2, dollar_table, &s2 );
		*success = *success || s2;
		start = string->value + len;
		if ( end && *end == '}' ) {
			int setting_len;
			char *name;
			
			*start = 0;
			*end++ = 0;
			name = start + 2;
			/* Determine setting length */
			setting_len = fetchf_named_setting ( name, NULL,
				0 );
			
			if ( setting_len < 0 )
				/* Treat error as empty setting */
				setting_len = 0;
			/* Read setting into buffer */
			{
				char expdollar[setting_len + 1];
				expdollar[0] = '\0';
				fetchf_named_setting ( name, expdollar,
					setting_len + 1 );
				if ( string3cat ( string, expdollar, end ) )
					end = string->value + len
						+ strlen ( expdollar );
			}
		} else {
			end = NULL;
			free_string ( string );
		}
		return end;
	} else if ( start[1] == '(' ) {
		{
			end = parse_arith ( string, start );
			return end;
		}
	}
	/* Can't find { so preserve the $ */
	end = start + 1;
	return end;
}

/**
 * @v charp	Pointer to start of characters to remove
 * @v num_del	Number of characters to remove
 *
 * Delete num_del characters, starting from charp
 */
static void delchar ( char *charp, int num_del ) {
	if ( charp )
		memmove ( charp, charp + num_del, strlen ( charp + num_del )
			+ 1);
}

/**
 * Deal with an escape sequence
 *
 * @v string	Input string
 * @v start	Point to start from
 * @ret end	Pointer to character after the escape
 *
 * Deal with a \.
 * \ at the end of a line will remove end of line
 * \<char> will remove the special meaning of <char>, if any
 */
char * parse_escape ( struct string *s, char *input, int *success ) {
	
	/* Found a \ at the end of the string => more input required */
	if ( ! input[1] ) {
		incomplete = 1;
		free_string ( s );
		return NULL;
	}
	if ( input[1] == '\n' ) {
		/* For a \ at end of line, remove both \ and \n */
		delchar ( input, 2 );
	} else {
		/* A \ removes the special meaning of any other character */
		delchar ( input, 1 );
		input++;
		*success = 1;
	}
	return input;
}

char * quotes_expand ( struct string *string, char *start, int *success ) {
	const struct char_table *table;
	char delim;
	
	delim = *start;
	switch ( delim ) {
		case '"':
			table = dquote_table;
		break;
		
		case '\'':
			table = squote_table;
		break;
		
		default:
			return NULL;
	}
	
	delchar ( start, 1 );	/* Remove the starting quotes */

	start = expand_string ( string, start, table, success );
	
	if ( start && *start != delim ) {
		incomplete = 1;
		start = NULL;
	}		
	delchar ( start, 1 );	/* Remove the ending quotes */
	
	DBG ( "string = [%s]\n", string->value );
	*success = 1;
	return start;
}

/**
 * Expand an input string
 *
 * @v string	Input as a struct string
 * @v start	Point to start from
 * @v table	Characters that have special meaning, and actions to take
 * @v success	Have we expanded anything?
 * @ret end	Pointer to first unconsumed character
 *
 * This is the main parsing function. The table contains a list of meaningful
 * characters, and the actions to be performed. The actions may be call a
 * function, or return a pointer to current character.
 */
char * expand_string ( struct string *string, char *start,
	const struct char_table *table, int *success ) {

	*success = 0;
	while ( 1 ) {
		const struct char_table * tline;
		char ch = *start;
		
		for ( tline = table; tline->token != 0; tline++ ) {
			/* Look for current token in the list we have */
			if ( tline->token == ch )
				break;
		}
		
		if ( tline->token != ch ) { /* If not found, copy into output */
			*success = 1;
			start++;
			continue;
		}
		
		if ( tline->parse_func == NULL )
			return start;
		start = tline->parse_func ( string, start, success );
		if ( !start )
			return NULL;		
	}
}
