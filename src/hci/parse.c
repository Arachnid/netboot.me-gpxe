#include <gpxe/parse.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gpxe/settings.h>

char * string3cat ( struct string *s1, const char *s2, const char *s3 ) { /* The len is the size of s1 */
	char *tmp = s1->value;
	asprintf ( &s1->value, "%s%s%s", s1->value, s2, s3 );
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
char * dollar_expand ( struct string *s, char *inp, char ** end ) {
	char *name;
	int setting_len;
	int len;
	
	len = inp - s->value;
	
	if ( inp[1] == '{' ) {
		*inp = 0;
		name = ( inp + 2 );

		/* Locate closer */
		*end = strstr ( name, "}" );
		if ( ! *end ) {
			printf ( "can't find ending }\n" );
			free_string ( s );
			return NULL;
		}
		**end = 0;
		*end += 1;
		
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
			if ( string3cat ( s, expdollar, *end ) )
				*end = s->value + len + strlen ( expdollar );
		}
		return s->value;
	} else if ( inp[1] == '(' ) {
		name = inp;
		{
			int ret;
			ret = parse_arith ( s, name, end );
			return s->value;		/* if ret < 0, s->value = NULL */
		}
	}
	/* Can't find { or (, so preserve the $ */
	*end = inp + 1;
	return s->value;
}

char * parse_escape ( struct string *s, char *input, char **end ) {
	char *exp;
	*input = 0;
	*end = input + 2;
	if ( input[1] == '\n' ) {
		int len = input - s->value;
		exp = stringcat ( s, *end );
		*end = exp + len;
	} else {
		int len = input - s->value;
		*end = input + 1;
		exp = stringcat ( s, *end );
		*end = exp + len + 1;
	}
	return exp;
}

/* Both *head and *end point somewhere within s */
char * expand_string ( struct string *s, char **head, char **end, const struct char_table *table, int tlen, int in_quotes, int *success ) {
	int i;
	char *cur_pos;
	int start;
	
	*success = 0;
	start = *head - s->value;
	cur_pos = *head;
	
	while ( *cur_pos ) {
		const struct char_table * tline = NULL;
		
		for ( i = 0; i < tlen; i++ ) {
			if ( table[i].token == *cur_pos ) {
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
					int pos = cur_pos - s->value;
					char *t;
					*success = 1;
					*cur_pos = 0;
					if ( ( t = stringcat ( s, cur_pos + 1 ) ) ) {
						*end = t + pos;
						*head = s->value + start;
					}
					return s->value;
				}
					break;
				case TABLE: /* 1 for recursive call. Probably found quotes */
					{
						int s2;
						char *end;
						char *tmp;
						
						*success = 1;
						*cur_pos = 0;
						tmp = s->value;
						stringcat ( s, cur_pos + 1 );
						cur_pos = s->value + ( cur_pos - tmp );
						if ( !expand_string ( s, &cur_pos, &end, tline->next.next_table.ntable, tline->next.next_table.len, 1, &s2 ) ) {
							return NULL;
						}
						cur_pos = end;
					}
					break;
				case FUNC: /* Call another function */
					{
						char *nstr;
						
						*success = 1;
						nstr = tline->next.parse_func ( s, cur_pos, &cur_pos );
						if ( !nstr ) {
							return NULL;
						}
					}
					break;
					
				case ENDTOK: /* End of input, and we also want next character */
					*end = cur_pos;
					*head = s->value + start;
					return s->value;
					break;
			}
		}
		
	}
	if ( in_quotes ) {
		printf ( "can't find closing '%c'\n", table[0].token );
		free_string ( s );
		return NULL;
	}
	*end = cur_pos;
	*head = s->value + start;
	return s->value;
}
