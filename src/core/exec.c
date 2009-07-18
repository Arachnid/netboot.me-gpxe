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
#include <gpxe/settings.h>

/** @file
 *
 * Command execution
 *
 */

/* Avoid dragging in getopt.o unless a command really uses it */
int optind;
int nextchar;

EXTERN_INIT_STACK ( if_stack, int, 10 );
size_t start_len;
size_t cur_len;
int incomplete;

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
	int rc = -1;

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
			if ( if_stack[COUNT ( if_stack )] == 1
				|| cmd->flags & 0x1 ) {
				
				rc = cmd->exec ( argc, ( char ** ) argv );
				if ( ! ( cmd->flags & 0x1 ) ) {
					store_rc ( rc );
				}
			} else
				rc = 0;
			return rc;
		}
	}
	printf ( "%s: command not found\n", command );
	return -ENOEXEC;
}

/** Expand a given command line and separate it into arguments */
static int expand_command ( const char *command,
	char **argv_stack, int *argv_count ) {
	
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
	
	/* Go through the command line and expand */
	while ( *head ) {
		while ( isspace ( *head ) ) { /* Ignore leading spaces */
			*head = 0;
			head++;
		}
		if ( *head == '#' ) {
			/* Comment is a new word that starts with # */
			break;
		}
		if ( !stringcpy ( &expcmd, head ) ) {
			argc = -ENOMEM;
			break;
		}
		head = expcmd.value;
		end = expand_string ( &expcmd, head, table,
			6, 0, &success );
		
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
	static char *complete_command;
	INIT_STACK ( argv_stack, char *, 20 );
	
	if ( !incomplete ) {
		start_len = cur_len;
		complete_command = strdup ( command );
	} else {
		char *tmp = complete_command;
		asprintf ( &complete_command, "%s\n%s",
			complete_command, command );
		free ( tmp );
	}
	incomplete = 0;
	if ( !complete_command ) {
		return -ENOMEM;
	}
	
	DBG ( "command = [%s]\n", complete_command );
	
	argc = expand_command ( complete_command, argv_stack,
		&COUNT ( argv_stack ) );
	
	if ( !incomplete ) {
		free ( complete_command );
		complete_command = NULL;
		if ( argc < 0 ) {
			rc = argc;
		} else {		
			PUSH_STACK ( argv_stack, NULL );
			if ( argc > 0 ) {
				rc = execv ( argv_stack[0], argv_stack );
				test_try ( rc );
			}
		}
	}
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
	.flags = 0,
};
