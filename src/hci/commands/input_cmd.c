#include <string.h>
#include <stdio.h>
#include <gpxe/command.h>
#include <gpxe/editbox.h>
#include <gpxe/input_ui.h>

FILE_LICENCE ( GPL2_OR_LATER );

static int input_exec ( int argc, char **argv ) {
	int rc;
	unsigned int flags = 0;

	switch (argc) {
		case 3:
			break;
		case 4:
			if ( !strcmp ( argv[1], "-p" ) ||
			     !strcmp ( argv[1], "--password" ) ) {
				flags |= EDITBOX_STARS;
				argv++;
				break;
			}
		default:
			printf ( "Usage: %s [-p|--password] setting prompt\n\n"
				 "  Prompt for user input string and store it "
				 "to a setting.\n", argv[0] );
			return 1;
	}

	if ( ( rc = input_ui(argv[1], argv[2], flags ) ) != 0 ) {
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
