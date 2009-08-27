#include <string.h>
#include <stdio.h>
#include <gpxe/command.h>
#include <gpxe/editbox.h>
#include <gpxe/input_ui.h>

FILE_LICENCE ( GPL2_OR_LATER );

static int input_exec ( int argc, char **argv ) {
	int rc;
	char *cmd_name;
	unsigned int flags = 0;

	cmd_name = argv[0];
	argc -= 1;
	argv++;
	if ( argc >= 1 && argv[0][0] == '-' && argv[0][1] == 'p' ) {
		flags |= EDITBOX_STARS;
		argc -= 1;
		argv++;
	}

	if ( argc != 2 ) {
		printf ( "Usage: %s [-p|--password] setting prompt\n"
             "\n"
						 "  Prompt for user input string and store it to a setting.\n",
						 cmd_name );
		return 1;
	}

	if ( ( rc = input_ui(argv[0], argv[1], flags ) ) != 0 ) {
		printf ( "Could not get input: %s\n",
			 strerror ( rc ) );
		return 1;
	}

	return 0;
}

struct command input_command __command = {
	.name = "input",
	.exec = input_exec,
};
