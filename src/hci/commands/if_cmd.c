#include <stdio.h>
#include <unistd.h>
#include <gpxe/command.h>
#include <gpxe/gen_stack.h>
#include <gpxe/init.h>

struct generic_stack if_stack = { .ptr = NULL, .tos = -1, .size = sizeof ( int ) };
static struct generic_stack else_stack = { .ptr = NULL, .tos = -1, .size = sizeof ( int ) };
int if_tos = 0;
struct generic_stack if_stack;
static struct generic_stack else_stack;
int isnum ( char *string, long *num );

static int if_exec ( int argc, char **argv ) {
	long cond;
	int zero = 0;
	if ( argc != 2 ) {
		printf ( "Syntax: if <condition>\n" );
		return 1;
	}
	
	if ( !isnum ( argv[1], &cond ) ) {
		printf ( "non-numeric condition: %s\n", argv[1] );
		return 1;
	}
	cond = TOP_GEN_STACK_INT ( &if_stack ) ? ( cond ? 1 : 0 ) : 0;
	if ( ( push_generic_stack ( &else_stack, &zero, 0 ) < 0 ) || ( push_generic_stack ( &if_stack, &cond, 0 ) < 0 ) ) {
		free_generic_stack ( &if_stack );
		free_generic_stack ( &else_stack );
		return 1;
	}

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
		return 1;
	}
	
	if ( if_stack.tos > 0 ) {	
		if ( pop_generic_stack ( &if_stack, &cond ) < 0 )
			return 1;
	} else {
		printf ( "fi without if\n" );
		return 1;
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
		return 1;
	}

	if ( TOP_GEN_STACK_INT ( &else_stack ) != 0 ) {
		printf ( "else without if\n" );
		return 1;
	}
	
	if ( ELEMENT_GEN_STACK_INT ( &if_stack, if_stack.tos - 1 ) )
		TOP_GEN_STACK_INT ( &if_stack ) = !TOP_GEN_STACK_INT ( &if_stack );
	
	return 0;
}

struct command else_command __command = {
	.name = "else",
	.exec = else_exec,
};

void init_if ( void ) {
	int one = 1;
	init_generic_stack ( &if_stack, sizeof ( int ) );
	init_generic_stack ( &else_stack, sizeof ( int ) );
	push_generic_stack ( &if_stack, &one, 0 );
	push_generic_stack ( &else_stack, &one, 0 );
	return;
}

struct init_fn initialise_if __init_fn ( INIT_NORMAL ) = {
	.initialise = init_if,
};