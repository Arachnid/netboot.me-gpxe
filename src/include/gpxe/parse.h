#ifndef _GPXE_PARSE_H_
#define _GPXE_PARSE_H_

#include <stdint.h>

#define ENDQUOTES 0
#define TABLE 1
#define FUNC 2
#define ENDTOK 3

/** Structure to store a string */
struct string {
	char *value;
};

/** Stores the important characters for parsing, and what to do with them */
struct char_table {
	/**
	 * Token
	 *
	 * Character to look out for.
	 */
	char token;
	/**
	 * Type
	 *
	 * What sort of character it is. Determines the action to be taken.
	 */
	int type;
	union {
		/** Recursively call the parsing function */
		struct {
			const struct char_table *ntable;
			int len; /* of ntable */
		} next_table;
		/**
		 * Parsing function
		 *
		 * @v string		String to be parsed, and also modified
		 * @v start		Pointer to current position in string
		 * @ret end		Pointer to first unconsumed character
		 */
		char * ( *parse_func ) ( struct string *string, char *start );
	}next; 
};

extern char * parse_arith ( struct string *inp, char *inp_str );
extern char * expand_string ( struct string *s, char *head,
	const struct char_table *table, int tlen, int in_quotes, int *success );

extern char * dollar_expand ( struct string *s, char *inp );
extern char * parse_escape ( struct string *s, char *input );
extern int isnum ( char *string, long *num );

/* Tables to be passed to expand_string */
extern const struct char_table dquote_table[3];
extern const struct char_table squote_table[1];
extern const struct char_table arith_table[22];
extern const struct char_table table[6];

extern int incomplete;

/* Functions to act on struct string */
extern void free_string ( struct string *s );
extern char * string3cat ( struct string *s1, const char *s2, const char *s3 );
extern char * stringcpy ( struct string *s1, const char *s2 );
extern char * stringcat ( struct string *s1, const char *s2 );

/* Functions for branches */
extern void store_rc ( int rc );
void test_try ( int rc );

#endif

