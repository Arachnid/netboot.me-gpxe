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
#include <gpxe/parse.h>
#include <gpxe/gen_stack.h>

#include <lib.h>

/** @file
 *
 * Command execution
 *
 */

/* Avoid dragging in getopt.o unless a command really uses it */
int optind;
int nextchar;

extern struct generic_stack if_stack;
extern int if_tos;

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
			if ( !if_stack.ptr || ( ( int * ) if_stack.ptr )[if_tos] || !strcmp ( cmd->name, "if" ) || !strcmp ( cmd->name, "fi" ) || !strcmp ( cmd->name, "else" ) )
				return cmd->exec ( argc, ( char ** ) argv );
			else
				return 0;
		}
	}

	printf ( "%s: command not found\n", command );
	return -ENOEXEC;
}

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

/**
 * Expand variables within command line
 *
 * @v command		Command line
 * @ret expcmd		Expanded command line
 *
 * The expanded command line is allocated with malloc() and the caller
 * must eventually free() it.
 */
int expand_command ( const char *command, struct generic_stack *argv_stack ) {
	char *expcmd;
	char *head, *end;
	char *nstring;
	int argc = 0;
	int success;
	
	init_generic_stack ( argv_stack, sizeof ( char * ) );
	
	/* Obtain temporary modifiable copy of command line */
	expcmd = strdup ( command );	
	if ( ! expcmd )
		return -ENOMEM;
	head = expcmd;
	
	/* Expand while expansions remain */
	while ( *head ) {
		while ( isspace ( *head ) )
			head++;
		if ( *head == '#' ) { /* Comment is a new word that starts with # */
			break;
		}
		nstring = expand_string ( head, &end, table, 6, 0, &success );
		if ( nstring ) {
			if ( success ) {
				char *cur_argv;
				argc++;
				cur_argv = calloc ( end - nstring + 1, 1);
			
				if ( cur_argv ) {
					strncpy ( cur_argv, nstring, end - nstring );
					if ( push_generic_stack ( argv_stack, &cur_argv, 0 ) < 0 ) {
						free ( cur_argv );
						free ( nstring );
						nstring = NULL;
					}
				} else {
					free ( nstring );
					nstring = NULL;
				}
			}
		}
		free ( expcmd );

		if ( !nstring ) {
			return -ENOMEM;
		}
		expcmd = nstring;
		head = end;
	}
	free ( expcmd );
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
	struct generic_stack argv_stack;

	argc = expand_command ( command, &argv_stack );
	
	if ( argc < 0 ) {
		rc = argc;
	} else {
		char **argv = realloc ( argv_stack.ptr, ( argc + 1 ) * sizeof ( char * ) );
		
		if ( argv ) {
			argv_stack.ptr = argv;
			argv[argc] = NULL;
			if ( argc > 0 )
				rc = execv ( argv[0], argv );
		} else
			rc = -ENOMEM;
	}
	
	free_generic_stack ( &argv_stack, 1 );
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
