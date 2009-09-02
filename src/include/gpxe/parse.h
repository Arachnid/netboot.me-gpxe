#ifndef _GPXE_PARSE_H_
#define _GPXE_PARSE_H_

#include <stdint.h>

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
	char * ( *parse_func ) ( struct string *string, char *start, int *success );
};

extern char * parse_arith ( struct string *inp, char *inp_str );
extern char * expand_string ( struct string *s, char *head,
	const struct char_table *table, int *success );

extern char * parse_escape ( struct string *s, char *input, int *success );
extern int isnum ( char *string, long *num );

/* Tables to be passed to expand_string */
extern const struct char_table dquote_table[4];
extern const struct char_table squote_table[2];
extern const struct char_table arith_table[23];
extern const struct char_table toplevel_table[7];

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

