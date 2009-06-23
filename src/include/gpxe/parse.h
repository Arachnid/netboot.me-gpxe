#ifndef _GPXE_PARSE_H_
#define _GPXE_PARSE_H_

#define			ENDQUOTES	0
#define			TABLE		1
#define			FUNC		2
#define			ENDTOK		3

struct char_table {
	char token;
	int type;
	union {
		struct {
			struct char_table *ntable;
			int len;
		} next_table;
		char * ( *parse_func ) ( char *, char ** );
	}next;
};

int parse_arith ( char *inp_string, char **end, char **buffer );

char * expand_string ( char * input, char **end, const struct char_table *table, int tlen, int in_quotes, int *success );
char * dollar_expand ( char *inp, char **end );
char * parse_escape ( char *input, char **end );
int isnum ( char *string, long *num );

extern struct char_table dquote_table[3];
extern struct char_table squote_table[1];

#endif