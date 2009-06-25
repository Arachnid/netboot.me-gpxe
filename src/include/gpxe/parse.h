#ifndef _GPXE_PARSE_H_
#define _GPXE_PARSE_H_

#include <stdint.h>

#define			ENDQUOTES	0
#define			TABLE		1
#define			FUNC		2
#define			ENDTOK		3

struct string {
	char *value;
};

struct char_table {
	char token;
	int type;
	union {
		struct {
			struct char_table *ntable;
			int len;
		} next_table;
		char * ( *parse_func ) ( struct string *, char *, char ** );
	}next;
};

int parse_arith ( struct string *inp, char *inp_str, char **end );

char * expand_string ( struct string *s, char **head, char **end, const struct char_table *table, int tlen, int in_quotes, int *success );
char * dollar_expand ( struct string *s, char *inp, char ** end );
char * parse_escape ( struct string *s, char *input, char **end );
int isnum ( char *string, long *num );

void free_string ( struct string *s );
char * string3cat ( struct string *s1, const char *s2, const char *s3 );
char * stringcpy ( struct string *s1, const char *s2 );
char * stringcat ( struct string *s1, const char *s2 );

extern struct char_table dquote_table[3];
extern struct char_table squote_table[1];

#endif