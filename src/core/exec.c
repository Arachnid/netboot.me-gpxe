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
#include <gpxe/command.h>
#include <gpxe/parse.h>
#include <gpxe/gen_stack.h>

/** @file
 *
 * Command execution
 *
 */

/* Avoid dragging in getopt.o unless a command really uses it */
int optind;
int nextchar;

EXTERN_INIT_STACK ( if_stack, int, 10 );
extern int COUNT ( loop_stack );
extern struct command_entry *cur_command;

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
			if ( if_stack[COUNT ( if_stack )] == 1 || !strcmp ( cmd->name, "if" ) || !strcmp ( cmd->name, "fi" ) || !strcmp ( cmd->name, "else" )
				|| !strcmp ( cmd->name, "while" ) || !strcmp ( cmd->name, "for" ) || !strcmp ( cmd->name, "done" ) )
				return cmd->exec ( argc, ( char ** ) argv );
			else
				return 0;
		}
	}

	printf ( "%s: command not found\n", command );
	return -ENOEXEC;
}

static int expand_command ( const char *command, char **argv_stack, int *argv_count ) {
	char *head, *end;
	int success;
	int argc;
	int i;
	
	INIT_STACK ( temp_stack, char *, 20 );
	
	struct string expcmd = { .value = NULL };
	
	argc = 0;
	
	if ( !stringcpy ( &expcmd, command ) ) {
		argc = -ENOMEM;
		return argc;
	}
	head = expcmd.value;
	
	/* Expand while expansions remain */
	while ( *head ) {
		while ( isspace ( *head ) ) {
			*head = 0;
			head++;
		}
		if ( *head == '#' ) { /* Comment is a new word that starts with # */
			break;
		}
		if ( !stringcpy ( &expcmd, head ) ) {
			argc = -ENOMEM;
			break;
		}
		head = expcmd.value;
		end = expand_string ( &expcmd, head, table, 6, 0, &success );
		if ( end ) {
			if ( success ) {
				char *argv = expcmd.value;
				argc++;
				expcmd.value = NULL;
				PUSH_STACK ( temp_stack, argv );
				if ( !stringcpy ( &expcmd, end ) ) {
					DBG ( "out of memory while pushing: %s\n", argv );
					argc = -ENOMEM;
					break;
				}
				*end = 0;
				/*
				So if the command is: word1 word2 word3
				argv_stack:	word1\0word2 word3
							word2\0word3
							word3
				*/
			}
		} else {
			argc = -ENOMEM;
			break;
		}
		head = expcmd.value;
	}
	for ( i = 0; i < 20; i++ )
		argv_stack[i] = temp_stack[i];
	*argv_count = COUNT ( temp_stack );
	free_string ( &expcmd );
	return argc;
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
	struct command_entry *c;
	INIT_STACK ( argv_stack, char *, 20 );
	DBG ( "command = [%s]\n", command );
	if ( cur_command == &start_command ) {
		if ( ( c = malloc ( sizeof ( struct command_entry ) + strlen ( command ) ) ) ) {
			strcpy ( c->line, command );
			list_add_tail ( &c->neighbours, &start_command.neighbours );
			DBG ( "stored: [%s]\n", c->line );
			cur_command = c;
		} else {
			DBG ( "failed to store [%s]\n", command );
			return -ENOMEM;
		}
	}
	argc = expand_command ( command, argv_stack, &COUNT ( argv_stack ) );
	if ( argc < 0 ) {
		rc = argc;
	} else {		
		PUSH_STACK ( argv_stack, NULL );
		if ( argc > 0 )
			rc = execv ( argv_stack[0], argv_stack );
	}
	cur_command = list_entry ( cur_command->neighbours.next, struct command_entry, neighbours );
	FREE_STACK_STRING ( argv_stack );
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
