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
/** @file
 *
 * Command execution
 *
 */

/* Avoid dragging in getopt.o unless a command really uses it */
int optind;
int nextchar;

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
		if ( strcmp ( command, cmd->name ) == 0 )
			return cmd->exec ( argc, ( char ** ) argv );
	}

	printf ( "%s: command not found\n", command );
	return -ENOEXEC;
}

struct argument {
	char *word;
	struct argument *next;
};

int parse_arith(char *inp_string, char **end, char **buffer);
char * parse_dquote ( char *inp, char **end );

char * dollar_expand ( char *inp );

char * parse_dquote ( char *inp, char **end ) {
	char *head;
	char *expquote;
	char *var;
	char *tmp;
	expquote = strdup ( inp );
	head = expquote;
	
	while ( *head && *head != '"' ) {
		switch ( *head ) {
			case '\\':
				if ( head[1] ) {
					if ( head[1] != '\n' ) {
						memmove ( head, head + 1, strlen ( head + 1 ) + 1 );
						head++;
					} else {
						memmove ( head, head + 2, strlen ( head + 2 ) + 1 );
					}
				}
				break;
			case '$':
				var = dollar_expand ( head );
				tmp = expquote;
				asprintf ( &expquote, "%s%s", expquote, var );
				head = expquote + ( head - tmp );
				free ( tmp );
				break;
			default:
				head++;
		}
	}
	*end = head;
	return expquote;
}

char * dollar_expand ( char *inp ) {
	char *expdollar;
	char *name;
	char *end;
	int setting_len;
	int new_len;
	if ( inp[1] == '{' ) {
		*inp = 0;
		name = ( inp + 2 );

		/* Locate closer */
		end = strstr ( name, "}" );
		if ( ! end )
			return NULL;
		*end++ = '\0';
		
		/* Determine setting length */
		setting_len = fetchf_named_setting ( name, NULL, 0 );
		if ( setting_len < 0 )
			setting_len = 0; /* Treat error as empty setting */

		/* Read setting into temporary buffer */
		{
			char setting_buf[ setting_len + 1 ];

			setting_buf[0] = '\0';
			fetchf_named_setting ( name, setting_buf,
					       sizeof ( setting_buf ) );

			/* Construct expanded string and discard old string */
			new_len = asprintf ( &expdollar, "%s%s", setting_buf, end );
			if ( new_len < 0 )
				return NULL;
		}
	} else if ( inp[1] == '(' ) {
		*inp = 0;
		name = ( inp + 1 );
		{
			int ret;
			char *arith_res;
			ret = parse_arith ( name, &end, &arith_res );
			
			if( ret < 0 ) {
				return NULL;
			}
			
			new_len = asprintf ( &expdollar, "%s%s", arith_res, end );
			if ( new_len < 0 )
				return NULL;
		}
	}
	return expdollar;	
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
	char *start;
	char *head;
	//char *name;
	//char *tail;
	//int setting_len;
	//int new_len;
	char *tmp;
	char *var;
	struct argument *argv;
	int argc = 0;

	/* Obtain temporary modifiable copy of command line */
	expcmd = strdup ( command );	
	if ( ! expcmd )
		return 0;
	
	*argv_start = malloc ( sizeof ( struct argument ) );
	if ( !*argv_start )
		return 0;
	argv = *argv_start;
	/* Expand while expansions remain */
	head = expcmd;
	while ( isspace ( *head ) )
		head++;
	start = head;
	while ( *head ) {
		switch ( *head ) {
			case '\\':
				if ( head[1] ) {
					if ( head[1] != '\n' ) {
						memmove ( head, head + 1, strlen ( head + 1 ) + 1 );
						head++;
					} else {
						memmove ( head, head + 2, strlen ( head + 2 ) + 1 );
					}
				}
				break;
			case '$':
				var = dollar_expand ( head );
				tmp = expcmd;
				asprintf ( &expcmd, "%s%s", expcmd, var );
				head = expcmd + ( head - tmp );
				start = expcmd + ( start - tmp );
				free ( tmp );
				break;
			case '"':
				var = parse_dquote ( head, &head );
				tmp = expcmd;
				asprintf ( &expcmd, "%s%s", expcmd, var );
				head = expcmd + ( head - tmp );
				start = expcmd + ( head - tmp );
				free ( tmp );
				break;
			case '\'':
				tmp = strchr ( head + 1, '\'' );
				memmove ( head, head + 1, tmp - head - 1 );
				memmove ( tmp - 1, tmp + 1, strlen ( tmp + 1 ) + 1 );
				head = tmp - 1;
				break;
			case ' ':
			case '\t':
				argv -> word = malloc ( head - start + 1 );
				strncpy ( argv -> word, start, head - start );
				argv -> word[head - start] = 0;
				argv -> next = malloc ( sizeof ( struct  argument ) ) ;
				while ( isspace ( *head ) )
					head++;
				
				printf ( "argv: %s %s\n", start, argv -> word );
				argv = argv -> next;
				start = head;
				printf ( "next: %s\n", start );
				argc++;
				break;
			default:
				head++;
		}
	}
	if ( start != head ) {
		argv -> word = strdup ( start );
		argv -> next = NULL;
		argc++;
	}
	else {
		free ( argv );
	}
	return argc;
}

/**
 * Split command line into argv array
 *
 * @v args		Command line
 * @v argv		Argument array to populate, or NULL
 * @ret argc		Argument count
 *
 * Splits the command line into whitespace-delimited arguments.  If @c
 * argv is non-NULL, any whitespace in the command line will be
 * replaced with NULs.
 */
#if 0
static int split_args ( char *args, char * argv[] ) {
	int argc = 0;

	while ( 1 ) {
		/* Skip over any whitespace / convert to NUL */
		while ( isspace ( *args ) ) {
			if ( argv )
				*args = '\0';
			args++;
		}
		/* Check for end of line */
		if ( ! *args )
			break;
		/* We have found the start of the next argument */
		if ( argv )
			argv[argc] = args;
		argc++;
		/* Skip to start of next whitespace, if any */
		while ( *args && ! isspace ( *args ) ) {
			args++;
		}
	}
	return argc;
}
#endif
/**
 * Execute command line
 *
 * @v command		Command line
 * @ret rc		Command exit status
 *
 * Execute the named command and arguments.
 */
int system ( const char *command ) {
	//char *args;
	int argc;
	int rc = 0;
	int i;
	struct argument *argv_start;
	struct argument *arg;
#if 0
	/* Perform variable expansion */
	args = expand_command ( command );
	if ( ! args )
		return -ENOMEM;

	/* Count arguments */
	argc = split_args ( args, NULL );

	/* Create argv array and execute command */
	if ( argc ) {
		char * argv[argc + 1];
		
		split_args ( args, argv );
		argv[argc] = NULL;

		if ( argv[0][0] != '#' )
			rc = execv ( argv[0], argv );
	}

	free ( args );
#endif
	argc = expand_command ( command, &argv_start );
	arg = argv_start;
	{
		char *argv[argc + 1];
		for ( i = 0; i < argc; i++ ) {
			argv[i] = arg -> word;
			arg = arg -> next;
			printf ( "argv[%d] = %s\n", i, argv[i] );
		}
		argv[i] = NULL;
		if ( argv[0][0] != '#' )
			rc = execv ( argv[0], argv );	
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
