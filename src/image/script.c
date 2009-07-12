/*
 * Copyright (C) 2007 Michael Brown <mbrown@fensystems.co.uk>.
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

/**
 * @file
 *
 * gPXE scripts
 *
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <gpxe/image.h>
#include <gpxe/parse.h>

struct image_type script_image_type __image_type ( PROBE_NORMAL );
extern size_t cur_len;
extern int command_source;
void init_if ();
size_t get_free_heap ( void );
int allow_bad;
/**
 * Execute script
 *
 * @v image		Script
 * @ret rc		Return status code
 */
static int script_exec ( struct image *image ) {
	size_t offset = 0;
	off_t eol;
	size_t len;
	int rc;
	int this_allow = 0;

	/* Temporarily de-register image, so that a "boot" command
	 * doesn't throw us into an execution loop.
	 */
	unregister_image ( image );
	/* Only one script can be executed at a time, so prevent stale loops from interfering */
	//if ( command_source == 1 )
	//	init_if ();
	command_source = 1;
	DBG ( "BEFORE: %i\n", get_free_heap () );
	while ( offset < image->len ) {
		/* Find length of next line, excluding any terminating '\n' */
		eol = memchr_user ( image->data, offset, '\n',
				    ( image->len - offset ) );
		if ( eol < 0 )
			eol = image->len;
		len = ( eol - offset );

		/* Copy line, terminate with NUL, and execute command */
		{
			char cmdbuf[ len + 1 ];
			

			copy_from_user ( cmdbuf, image->data, offset, len );
			cmdbuf[len] = '\0';
			cur_len = offset;
			DBG ( "$ %s\n", cmdbuf );
			if ( ( rc = system ( cmdbuf ) ) && ! this_allow ) {
				DBG ( "Command \"%s\" failed: %s\n",
				      cmdbuf, strerror ( rc ) );
				goto done;
			}
		}
		if ( offset == 0 )
			this_allow = allow_bad;
		else
			allow_bad = this_allow;
		/* Move to next line */
		if ( offset == cur_len )		
			offset += ( len + 1 );
		else /* A done statement changed the offset */
			offset = cur_len;
	}

	rc = 0;
 done:
	DBG ( "AFTER: %i\n", get_free_heap () );
	/* Re-register image and return */
	register_image ( image );
	return rc;
}

/**
 * Load script into memory
 *
 * @v image		Script
 * @ret rc		Return status code
 */
static int script_load ( struct image *image ) {
	static const char magic[] = "#!gpxe";
	char test[ sizeof ( magic ) - 1 /* NUL */ + 1 /* terminating space */];
	static const char noexit_option[] = "--no-exit";
	char option_test[ sizeof ( noexit_option ) ];
	const char *ptr;

	/* Sanity check */
	if ( image->len < sizeof ( test ) ) {
		DBG ( "Too short to be a script\n" );
		return -ENOEXEC;
	}

	/* Check for magic signature */
	copy_from_user ( test, image->data, 0, sizeof ( test ) );
	if ( ( memcmp ( test, magic, ( sizeof ( test ) - 1 ) ) != 0 ) ||
	     ! isspace ( test[ sizeof ( test ) - 1 ] ) ) {
		DBG ( "Invalid magic signature\n" );
		return -ENOEXEC;
	}
	allow_bad = 0;
	/* Check for a --no-exit option */
	ptr = ( const char * ) image->data + strlen ( magic );
	while ( *ptr == ' ' || *ptr == '\t' )
		ptr++;
	strncpy ( option_test, ptr, strlen ( noexit_option ) );
	option_test[strlen ( noexit_option ) ] = 0;
	if ( !strcmp ( noexit_option, option_test ) && isspace ( ptr[strlen ( noexit_option )] ) ) {
		allow_bad = 1;
		DBG ( "--no-exit option\n" );
	}
	
	/* This is a script */
	image->type = &script_image_type;

	/* We don't actually load it anywhere; we will pick the lines
	 * out of the image as we need them.
	 */

	return 0;
}

/** Script image type */
struct image_type script_image_type __image_type ( PROBE_NORMAL ) = {
	.name = "script",
	.load = script_load,
	.exec = script_exec,
};
