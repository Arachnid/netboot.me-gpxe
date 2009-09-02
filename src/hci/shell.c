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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <readline/readline.h>
#include <gpxe/command.h>
#include <gpxe/shell.h>
#include <gpxe/parse.h>

#include <hci/if_cmd.h>

/** @file
 *
 * Minimal command shell
 *
 */

/** The shell prompt string */
static const char shell_prompt[] = "gPXE> ";

int command_source;
size_t get_free_heap ( void );
/** Flag set in order to exit shell */
static int exit_flag = 0;

/** "exit" command body */
static int exit_exec ( int argc, char **argv __unused ) {

	if ( argc == 1 ) {
		exit_flag = 1;
	} else {
		printf ( "Usage: exit\n"
			 "Exits the command shell\n" );
	}

	return 0;
}

/** "exit" command definition */
struct command exit_command __command = {
	.name = "exit",
	.exec = exit_exec,
};

/** "help" command body */
static int help_exec ( int argc __unused, char **argv __unused ) {
	struct command *command;
	unsigned int hpos = 0;

	printf ( "\nAvailable commands:\n\n" );
	for_each_table_entry ( command, COMMANDS ) {
		hpos += printf ( "  %s", command->name );
		if ( hpos > ( 16 * 4 ) ) {
			printf ( "\n" );
			hpos = 0;
		} else {
			while ( hpos % 16 ) {
				printf ( " " );
				hpos++;
			}
		}
	}
	printf ( "\n\nType \"<command> --help\" for further information\n\n" );
	return 0;
}

/** "help" command definition */
struct command help_command __command = {
	.name = "help",
	.exec = help_exec,
};

/**
 * Start command shell
 *
 */
void shell ( void ) {
	char *line;
	size_t offset = 0;
	struct string input = { .value = 0 };
	exit_flag = 0;
	init_if();
	printf ( "BEFORE: %i\n", get_free_heap () );
	while ( ! exit_flag ) {
		int len;
		command_source = 0;
		if ( input.value && offset < strlen ( input.value ) ) {
			len = strcspn ( input.value + offset, "\n" );
			line = strndup ( input.value + offset, len );
		} else {
			offset = input.value ? strlen ( input.value ) : 0;
			line = readline ( shell_prompt
				+ ( incomplete ? 4 : 0 ) );
			string3cat ( &input, line, "\n"	);
		}
		if ( line ) {
			cur_len = offset;
			if ( !incomplete )
				start_len = cur_len;
			DBG ( "executing %d:[%s]\n", offset, line );
			system ( line );
			if ( cur_len == offset || command_source != 0 )
				offset += strlen ( line ) + 1;
			else
				offset = cur_len;
			free ( line );
			DBG ( "stored: %s", input.value );
			DBG ( "new offset = %d\n", offset );
		} else
			break;
	}
	free_string ( &input );
	printf ( "AFTER: %i\n", get_free_heap () );
}
