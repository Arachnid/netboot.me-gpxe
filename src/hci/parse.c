#include <gpxe/parse.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gpxe/settings.h>

/* The returned string is allocated on the heap and the calling function must free it */
char * dollar_expand ( char *inp, char ** end ) {
	char *expdollar;
	const char *name;
	int setting_len;
	
	if ( inp[1] == '{' ) {
		name = ( inp + 2 );

		/* Locate closer */
		*end = strstr ( name, "}" );
		if ( ! *end ) {
			printf ( "can't find ending }\n" );
			return NULL;
		}
		**end = 0;
		*end += 1;
		
		/* Determine setting length */
		setting_len = fetchf_named_setting ( name, NULL, 0 );
		if ( setting_len < 0 )
			setting_len = 0; /* Treat error as empty setting */

		/* Read setting into buffer */
		expdollar = malloc ( setting_len + 1 );
		if ( expdollar ) {
			expdollar[0] = '\0';
			fetchf_named_setting ( name, expdollar,
							setting_len + 1 );
		}
		return expdollar;
		
	} else if ( inp[1] == '(' ) {
		name = ( inp + 1 );
		{
			int ret;
			ret = parse_arith ( name, end, &expdollar );
			
			if( ret < 0 ) {
				return NULL;
			}
			
			return expdollar;
		}
	}
	/* Can't find { or (, so preserve the $ */
	*end = inp + 1;
	asprintf ( &expdollar, "%c", '$' );
	return expdollar;
}

/* The returned string is allocated on the heap and the calling function must free it */
char * parse_escape ( char *input, char **end ) {
	char *exp;
	*end = input + 2;
	if ( input[1] == '\n' ) {
		exp = malloc ( 1 );
		if ( exp )
			*exp = 0;
	} else {
		asprintf ( &exp, "%c", input[1] );
	}
	return exp;
}

/* The returned string is allocated on the heap and the calling function must free it */
char * expand_string ( const char * input, char **end, const struct char_table *table, int tlen, int in_quotes, int *success ) {
	char *expstr;
	char *head;
	char *tmp;
	int i;
	int new_len;
	
	*success = 0;
	expstr = strdup ( input );
	
	head = expstr;
	
	while ( *head ) {
		const struct char_table * tline = NULL;
		
		for ( i = 0; i < tlen; i++ ) {
			if ( table[i].token == *head ) {
				tline = table + i;
				break;
			}
		}
		
		if ( ! tline ) {
			*success = 1;
			head++;
		} else {
			switch ( tline->type ) {
				case ENDQUOTES: /* 0 for end of input, where next char is to be discarded. Used for ending ' or " */
					*success = 1;
					*head = 0;
					*end = head + 1;
					return expstr;
					break;
				case TABLE: /* 1 for recursive call. Probably found quotes */
				case FUNC: /* Call another function */
					{
						char *nstr;
						int s2;
						
						*success = 1;
						*head = 0;
						nstr = ( tline->type == TABLE ) ? ( expand_string ( head + 1, &head, tline->next.next_table.ntable, tline->next.next_table.len, 1, &s2 ) ) 
													: ( tline->next.parse_func ( head, &head ) );
						tmp = expstr;
						new_len = asprintf ( &expstr, "%s%s%s", expstr, nstr, head );
						head = expstr + strlen ( tmp ) + strlen ( nstr );
						
						free ( tmp );
						free ( nstr );
						if ( !nstr || new_len < 0 ) { /* if new_len < 0, expstr = NULL */
							free ( expstr );
							return NULL;
						}
					}
					break;
				case ENDTOK: /* End of input, and we also want next character */
					*end = head;
					return expstr;
					break;
			}
		}
		
	}
	if ( in_quotes ) {
		printf ( "can't find closing '%c'\n", table[0].token );
		free ( expstr );
		return NULL;
	}
	*end = head;
	return expstr;
}
