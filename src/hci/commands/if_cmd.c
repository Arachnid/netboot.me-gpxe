#include <stdio.h>
#include <unistd.h>
#include <gpxe/command.h>
#include <gpxe/gen_stack.h>

#include <lib.h>

struct generic_stack if_stack = { .ptr = NULL, .tos = -1, .size = sizeof ( int ) };
static struct generic_stack else_stack = { .ptr = NULL, .tos = -1, .size = sizeof ( int ) };
int if_tos = 0;
int isnum ( char *string, long *num );

static int if_exec ( int argc, char **argv ) {
	long cond;
	int zero = 0;
	if ( argc != 2 ) {
		printf ( "Syntax: if <condition>\n" );
		return -1;
	}
	
	if ( !isnum ( argv[1], &cond ) ) {
		printf ( "non-numeric condition\n" );
		return -1;
	}
	cond = cond ? 1 : 0;
	push_generic_stack ( &else_stack, &zero, 0 );
	push_generic_stack ( &if_stack, &cond, 0 );
	
	if ( ( ( int * ) if_stack.ptr )[if_tos] != 0 )
		if_tos++;
	
	//printf ( "Condition is %ld\n", cond );
	return 0;
}

struct command if_command __command = {
	.name = "if",
	.exec = if_exec,
};

static int fi_exec ( int argc, char **argv ) {
	int cond;
	if ( argc != 1 ) {
		printf ( "Syntax: %s\n", argv[0] );
		return -1;
	}
	
	if ( pop_generic_stack ( &if_stack, &cond ) == 0 ) {
		if ( if_tos - 1 == if_stack.tos ) {
			if_tos = if_stack.tos;
		}
	} else {
		printf ( "fi without if\n" );
		return -1;
	}
	return 0;		
}


struct command fi_command __command = {
	.name = "fi",
	.exec = fi_exec,
};

static int else_exec ( int argc, char **argv ) {
	if ( argc != 1 ) {
		printf ( "Syntax: %s\n", argv[0] );
	}

	if ( ( ( int * ) else_stack.ptr )[else_stack.tos] != 0 ) {
		printf ( "else without if\n" );
		return -1;
	}
	
	if ( if_tos == if_stack.tos ) {
		( ( int * ) if_stack.ptr )[if_stack.tos] = !( ( ( int * ) if_stack.ptr )[if_stack.tos] );
	}
	return 0;
}

struct command else_command __command = {
	.name = "else",
	.exec = else_exec,
};