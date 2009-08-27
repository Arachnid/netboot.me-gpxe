#include <string.h>
#include <stdio.h>
#include <gpxe/command.h>
#include <gpxe/editbox.h>
#include <gpxe/input_ui.h>

FILE_LICENCE ( GPL2_OR_LATER );

static int login_exec ( int argc, char **argv ) {
	int rc;

	if ( argc > 1 ) {
		printf ( "Usage: %s\n"
			 "Prompt for login credentials\n", argv[0] );
		return 1;
	}

	if ( ( rc = input_ui( "username", "Username:", 0 ) ) != 0 ) {
		printf ( "Could not get username: %s\n", strerror ( rc ) );
		return 1;
	}

	if ( ( rc = input_ui( "password", "Password:", EDITBOX_STARS ) )  != 0 ) {
		printf ( "Could not get username: %s\n", strerror ( rc ) );
		return 1;
	}

	return 0;
}

struct command login_command __command = {
	.name = "login",
	.exec = login_exec,
};
