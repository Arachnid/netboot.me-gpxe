/*
 * Copyright (C) 2006 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <assert.h>
#include <gpxe/tables.h>
#include <gpxe/command.h>
#include <gpxe/settings.h>

#include <lib.h>


#define			ENDQUOTES	0
#define			TABLE		1
#define			FUNC		2
#define			ENDTOK		3

/** @file
 *
 * Command execution
 *
 */

/* Avoid dragging in getopt.o unless a command really uses it */
int optind;
int nextchar;

extern int branch_stack[10];
extern int branch_tos;

/**
 * Execute command
 *
 * @v command		Command name
 * @v argv		Argument list
 * @ret rc		Command exit status
 *
 * Execute the named command.  Unlike a traditional POSIX execv(),
 * this function returns the exit status of the command.
 */
int execv ( const char *command, char * const argv[] ) {
	struct command *cmd;
	int argc;

	/* Count number of arguments */
	for ( argc = 0 ; argv[argc] ; argc++ ) {}

	/* Sanity checks */
	if ( ! command ) {
		DBG ( "No command\n" );
		return -EINVAL;
	}
	if ( ! argc ) {
		DBG ( "%s: empty argument list\n", command );
		return -EINVAL;
	}

	/* Reset getopt() library ready for use by the command.  This
	 * is an artefact of the POSIX getopt() API within the context
	 * of Etherboot; see the documentation for reset_getopt() for
	 * details.
	 */
	reset_getopt();

	/* Hand off to command implementation */
	for_each_table_entry ( cmd, COMMANDS ) {
		if ( strcmp ( command, cmd->name ) == 0 ) {
			if ( branch_stack[branch_tos] || !strcmp ( cmd -> name, "if" ) || !strcmp ( cmd -> name, "fi" ) || !strcmp ( cmd -> name, "else" ) )
				return cmd->exec ( argc, ( char ** ) argv );
			else
				return 0;
		}
	}

	printf ( "%s: command not found\n", command );
	return -ENOEXEC;
}

struct argument {
	char *word;
	struct argument *next;
};

int parse_arith(char *inp_string, char **end, char **buffer);

char * dollar_expand ( char *inp, char **end ) {
	char *expdollar;
	char *name;
	int setting_len;
	
	*inp = 0;
	if ( inp[1] == '{' ) {
		name = ( inp + 2 );

		/* Locate closer */
		*end = strstr ( name, "}" );
		if ( ! *end ) {
			printf ( "can't find ending }\n" );
			return NULL;
		}
		**end = '\0';
		*end += 1;
		
		/* Determine setting length */
		setting_len = fetchf_named_setting ( name, NULL, 0 );
		if ( setting_len < 0 )
			setting_len = 0; /* Treat error as empty setting */

		/* Read setting into temporary buffer */
		{
			char *setting_buf = malloc ( setting_len + 1 );

			setting_buf[0] = '\0';
			fetchf_named_setting ( name, setting_buf,
					       setting_len + 1 );
			return setting_buf;
		}
	} else if ( inp[1] == '(' ) {
		name = ( inp + 1 );
		{
			int ret;
			char *arith_res;
			ret = parse_arith ( name, end, &arith_res );
			
			if( ret < 0 ) {
				return NULL;
			}
			
			return arith_res;
		}
	}
	/* Can't find { or (, so preserve the $ (current behaviour) */
	*end = inp + 1;
	asprintf ( &expdollar, "%c", '$' );
	return expdollar;
}

char * parse_escape ( char *input, char **end ) {
	char *exp;
	*input = 0;
	if ( input[1] == '\n' ) {
		*end = input + 2;
		exp = malloc ( 1 );
		*exp = 0;
	} else {
		*end = input + 2;
		asprintf ( &exp, "%c", input[1] );
	}
	return exp;
}


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

struct char_table dquote_table[3] = {
	{ .token = '"', .type = ENDQUOTES },
	{ .token = '$', .type = FUNC, .next.parse_func = dollar_expand },
	{ .token = '\\', .type = FUNC, .next.parse_func = parse_escape }
};
struct char_table squote_table[1] = {
	{ .token = '\'', .type = ENDQUOTES }
};
static struct char_table table[6] = {
	{ .token = '\\', .type = FUNC, .next.parse_func = parse_escape },
	{ .token = '"', .type = TABLE, .next = 
				{.next_table = { .ntable = dquote_table, .len = 3 } } },
	{ .token = '$', .type = FUNC, .next.parse_func = dollar_expand },
	{ .token = '\'', .type = TABLE, .next = { .next_table = { .ntable = squote_table, .len = 1 } } },
	{ .token = ' ', .type = ENDTOK },
	{ .token = '\t', .type = ENDTOK }
};

char * expand_string ( char * input, char **end, const struct char_table *table, int tlen, int in_quotes ) {
	char *expstr;
	char *head;
	char *nstr, *tmp;
	int i;
	int new_len;
	
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
			head++;
		} else {
			switch ( tline -> type ) {
				case ENDQUOTES: /* 0 for end of input, where next char is to be discarded. Used for ending ' or " */
					*head = 0;
					*end = head + 1;
					//printf ( "return value: [%s]\n", expstr );
					return expstr;
					break;
				case TABLE: /* 1 for recursive call. Probably found quotes */
				case FUNC: /* Call another function */
					{
						*head = 0;
						nstr = ( tline -> type == TABLE ) ? ( expand_string ( head + 1, &head, tline->next.next_table.ntable, tline->next.next_table.len, 1 ) ) 
													: ( tline -> next.parse_func ( head, &head ) );
						tmp = expstr;
						
						new_len = asprintf ( &expstr, "%s%s%s", expstr, nstr, head );
						if ( in_quotes || tline -> type == TABLE )
							head = expstr + strlen ( tmp ) + strlen ( nstr );
						else
							head = expstr + strlen ( tmp );
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

/**
 * Expand variables within command line
 *
 * @v command		Command line
 * @ret expcmd		Expanded command line
 *
 * The expanded command line is allocated with malloc() and the caller
 * must eventually free() it.
 */
int expand_command ( const char *command, struct argument **argv_start ) {

	char *expcmd;
	char *head, *end;
	char *nstring;
	struct argument *cur_arg = NULL;
	int argc = 0;
	
	/* Obtain temporary modifiable copy of command line */
	expcmd = strdup ( command );	
	if ( ! expcmd )
		return -ENOMEM;
	*argv_start = NULL;
	head = expcmd;
	/* Expand while expansions remain */
	while ( *head ) {
		while ( isspace ( *head ) )
			head++;
		if ( *head == '#' && !*argv_start ) { /* Comment starts with # */
			return 0;
		}
		nstring = expand_string ( head, &end, table, 6, 0 );
		if ( !nstring ) {
			while ( *argv_start ) {
				cur_arg = *argv_start;
				*argv_start = ( *argv_start ) -> next;
				free ( cur_arg );
			}
			return -ENOMEM;
		}
		if ( nstring != end ) {
			argc++;
			if ( !*argv_start ) {
				*argv_start = calloc ( sizeof ( struct argument ), 1 );
				cur_arg = *argv_start;
			} else {
				cur_arg -> next = calloc ( sizeof ( struct argument ), 1 );
				cur_arg = cur_arg -> next;
			}
		
			if ( !cur_arg ) {
				while ( *argv_start ) {
					cur_arg = *argv_start;
					*argv_start = ( *argv_start ) -> next;
					free ( cur_arg );
				}
				return -ENOMEM;
			}
		
			cur_arg -> word = calloc ( end - nstring + 1, 1);
			strncpy ( cur_arg -> word, nstring, end - nstring );
		}
		free ( expcmd );
		expcmd = nstring;
		head = end;
	}
	return argc;

out_of_memory:
	while ( *argv_start != argv ) {
		struct argument *tmp;
		tmp = *argv_start;
		*argv_start = ( *argv_start ) -> next;
	}
	free ( argv );
	return -ENOMEM;
}

/**
 * Execute command line
 *
 * @v command		Command line
 * @ret rc		Command exit status
 *
 * Execute the named command and arguments.
 */
int system ( const char *command ) {
	int argc;
	int rc = 0;
	int i;
	struct argument *argv_start;
	struct argument *arg;

	argc = expand_command ( command, &argv_start );
	
	if ( argc < 0 )
		return -ENOMEM;

	arg = argv_start;
	{
		char *argv[argc + 1];
		for ( i = 0; i < argc; i++ ) {
			struct argument *tmp;
			tmp = arg;
			argv[i] = arg -> word;
			arg = arg -> next;
			free ( tmp );
			
			//printf ( "[%s] ", argv[i] );
		}
		argv[i] = NULL;
		//printf ( "\n" );
		
		if ( argc > 0 )
			rc = execv ( argv[0], argv );
		for ( i = 0; i < argc; i++)
			free ( argv[i] );
	}
	return rc;
}

/**
 * The "echo" command
 *
 * @v argc		Argument count
 * @v argv		Argument list
 * @ret rc		Exit code
 */
static int echo_exec ( int argc, char **argv ) {
	int i;

	for ( i = 1 ; i < argc ; i++ ) {
		printf ( "%s%s", ( ( i == 1 ) ? "" : " " ), argv[i] );
	}
	printf ( "\n" );
	return 0;
}

/** "echo" command */
struct command echo_command __command = {
	.name = "echo",
	.exec = echo_exec,
};
