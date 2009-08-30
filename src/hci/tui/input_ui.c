/*
 * Copyright (C) 2009 Michael Brown <mbrown@fensystems.co.uk>,
 *                    Nick Johnson <arachnid@notdot.net>.
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

/** @file
 *
 * Input box UI
 *
 */

#include <string.h>
#include <errno.h>
#include <curses.h>
#include <console.h>
#include <gpxe/settings.h>
#include <gpxe/editbox.h>
#include <gpxe/keys.h>
#include <gpxe/input_ui.h>

/* Colour pairs */
#define CPAIR_NORMAL		1
#define CPAIR_LABEL		2
#define CPAIR_EDITBOX		3

/* Screen layout */
#define INPUT_LABEL_ROW	11
#define INPUT_ROW		13
#define LABEL_CENTER		40
#define EDITBOX_COL		30
#define EDITBOX_WIDTH		20

int input_ui ( char *setting_name, char *prompt, unsigned int flags ) {
	char input[256];
	struct edit_box input_box;
	int key;
	int rc = -EINPROGRESS;
	int label_col = LABEL_CENTER - strlen ( prompt ) / 2;

	if ( label_col < 0 )
		label_col = 0;

	/* Fetch current setting value */
	if ( fetchf_named_setting ( setting_name, input, sizeof ( input ) )
		<= 0 )
		input[0] = '\0';

	/* Initialise UI */
	initscr();
	start_color();
	init_pair ( CPAIR_NORMAL, COLOR_WHITE, COLOR_BLACK );
	init_pair ( CPAIR_LABEL, COLOR_WHITE, COLOR_BLACK );
	init_pair ( CPAIR_EDITBOX, COLOR_WHITE, COLOR_BLUE );
	init_editbox ( &input_box, input, sizeof ( input ), NULL,
		       INPUT_ROW, EDITBOX_COL, EDITBOX_WIDTH, flags );

	/* Draw initial UI */
	erase();
	color_set ( CPAIR_LABEL, NULL );
	mvprintw ( INPUT_LABEL_ROW, label_col, prompt );
	color_set ( CPAIR_EDITBOX, NULL );
	draw_editbox ( &input_box );

	/* Main loop */
	while ( rc == -EINPROGRESS ) {

		draw_editbox ( &input_box );

		key = getkey();
		switch ( key ) {
		case KEY_ENTER:
			rc = 0;
			break;
		case CTRL_C:
		case ESC:
			rc = -ECANCELED;
			break;
		default:
			edit_editbox ( &input_box, key );
			break;
		}
	}

	/* Terminate UI */
	color_set ( CPAIR_NORMAL, NULL );
	erase();
	endwin();

	if ( rc != 0 )
		return rc;

	/* Store settings */
	if ( ( rc = storef_named_setting ( setting_name, input ) ) != 0 )
		return rc;

	return 0;
}
