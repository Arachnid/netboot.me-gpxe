#include <gpxe/parse.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gpxe/settings.h>

extern int incomplete;

const struct char_table arith_table[22] = {
	{ .token = '\\', .type = FUNC, .next.parse_func = parse_escape },
	{ .token = '"', .type = TABLE, .next = { .next_table = { .ntable = dquote_table, .len = 3 } } },
	{ .token = '$', .type = FUNC, .next.parse_func = dollar_expand },
	{ .token = '\'', .type = TABLE, .next = { .next_table = { .ntable = squote_table, .len = 1 } } },
	{ .token = ' ', .type = ENDQUOTES },
	{ .token = '\t', .type = ENDQUOTES },
	{ .token = '\n', .type = ENDQUOTES },
	{ .token = '~', .type = ENDTOK },
	{ .token = '!', .type = ENDTOK },
	{ .token = '*', .type = ENDTOK },
	{ .token = '/', .type = ENDTOK },
	{ .token = '%', .type = ENDTOK },
	{ .token = '+', .type = ENDTOK },
	{ .token = '-', .type = ENDTOK },
	{ .token = '<', .type = ENDTOK },
	{ .token = '=', .type = ENDTOK },
	{ .token = '>', .type = ENDTOK },
	{ .token = '&', .type = ENDTOK },
	{ .token = '|', .type = ENDTOK },
	{ .token = '^', .type = ENDTOK },
	{ .token = '(', .type = ENDTOK },
	{ .token = ')', .type = ENDTOK }
};

const struct char_table dquote_table[3] = {
	{ .token = '"', .type = ENDQUOTES },
	{ .token = '$', .type = FUNC, .next.parse_func = dollar_expand },
	{ .token = '\\', .type = FUNC, .next.parse_func = parse_escape }
};
const struct char_table squote_table[1] = {
	{ .token = '\'', .type = ENDQUOTES }
};
const struct char_table table[6] = {
	{ .token = '\\', .type = FUNC, .next.parse_func = parse_escape },
	{ .token = '"', .type = TABLE, .next = 
				{.next_table = { .ntable = dquote_table, .len = 3 } } },
	{ .token = '$', .type = FUNC, .next.parse_func = dollar_expand },
	{ .token = '\'', .type = TABLE, .next = { .next_table = { .ntable = squote_table, .len = 1 } } },
	{ .token = ' ', .type = ENDTOK },
	{ .token = '\t', .type = ENDTOK }
};

const struct char_table dollar_table[2] = {
	{ .token = '}', .type = ENDTOK },
	{ .token = '$', .type = FUNC, .next.parse_func = dollar_expand },
};

char * string3cat ( struct string *s1, const char *s2, const char *s3 ) { /* The len is the size of s1 */
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

char * stringcpy ( struct string *s1, const char *s2 ) {
	char *tmp = s1->value;
	s1->value = strdup ( s2 );
	free ( tmp );
	return s1->value;
}

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

void free_string ( struct string *s ) {
	free ( s->value );
	s->value = NULL;
}

/* struct string *s:
Before: [start]$<exp>[end]
End: [start]<expanded>[end]
and *end points to [end]
*/
char * dollar_expand ( struct string *s, char *inp ) {
	char *name;
	int setting_len;
	int len;
	char *end;
	
	len = inp - s->value;

	if ( inp[1] == '{' ) {
		int success;
		end = expand_string ( s, inp + 2, dollar_table, 2, 1, &success );
		inp = s->value + len;
		if ( end ) {
			*inp = 0;
			*end++ = 0;
			name = inp + 2;
			/* Determine setting length */
			setting_len = fetchf_named_setting ( name, NULL, 0 );
			if ( setting_len < 0 )
				setting_len = 0; /* Treat error as empty setting */
			
			/* Read setting into buffer */
			{
				char expdollar[setting_len + 1];
				expdollar[0] = '\0';
				fetchf_named_setting ( name, expdollar,
								setting_len + 1 );
				if ( string3cat ( s, expdollar, end ) )
					end = s->value + len + strlen ( expdollar );
			}
		}
		return end;
	} else if ( inp[1] == '(' ) {
		name = inp;
		{
			end = parse_arith ( s, name );
			return end;
		}
	}
	/* Can't find { or (, so preserve the $ */
	end = inp + 1;
	return end;
}

char * parse_escape ( struct string *s, char *input ) {
	char *exp;
	char *end;
	
	if ( ! input[1] ) {
		incomplete = 1;
		free_string ( s );
		return NULL;
	}
	*input = 0;
	end = input + 2;
	if ( input[1] == '\n' ) {
		int len = input - s->value;
		exp = stringcat ( s, end );
		end = exp + len;
	} else {
		int len = input - s->value;
		end = input + 1;
		exp = stringcat ( s, end );
		end = exp + len + 1;
	}
	return end;
}

/* Return a pointer to the first unconsumed character */
char * expand_string ( struct string *s, char *head, const struct char_table *table, int tlen, int in_quotes, int *success ) {
	int i;
	int cur_pos;	/* s->value may be reallocated, so this seems better */
	
	*success = 0;
	cur_pos = head - s->value;
	
	while ( s->value[cur_pos] ) {
		const struct char_table * tline = NULL;
		
		for ( i = 0; i < tlen; i++ ) {
			if ( table[i].token == s->value[cur_pos] ) {
				tline = table + i;
				break;
			}
		}
		
		if ( ! tline ) {
			*success = 1;
			cur_pos++;
		} else {
			switch ( tline->type ) {
				case ENDQUOTES: /* 0 for end of input, where next char is to be discarded. Used for ending ' or " */
				{
					*success = 1;
					s->value[cur_pos] = 0;
					if ( ! stringcat ( s, s->value + cur_pos + 1 ) )
						return NULL;
					return s->value + cur_pos;
				}
					break;
				case TABLE: /* 1 for recursive call. Probably found quotes */
					{
						int s2;
						char *tmp;
						
						*success = 1;
						s->value[cur_pos] = 0;
						/* Remove the current character and call recursively */
						if ( !stringcat ( s, s->value + cur_pos + 1 ) )
							return NULL;
						
						tmp = expand_string ( s, s->value + cur_pos, tline->next.next_table.ntable, tline->next.next_table.len, 1, &s2 );
						if ( ! tmp )
							return NULL;
						cur_pos = tmp - s->value;
					}
					break;
				case FUNC: /* Call another function */
					{
						char *tmp;
						if ( ! ( tmp = tline->next.parse_func ( s, s->value + cur_pos ) ) )
							return NULL;
						if ( tmp - s->value != cur_pos )
							*success = 1;
						cur_pos = tmp - s->value;
					}
					break;
					
				case ENDTOK: /* End of input, and we also want next character */
					return s->value + cur_pos;
					break;
			}
		}
		
	}
	if ( in_quotes ) {
		incomplete = 1;
		free_string ( s );
		return NULL;
	}
	return s->value + cur_pos;
}
