#include <stdio.h>
#include <gpxe/command.h>

int branch_stack[10] = { 1 };
int branch_tos = 0;
static int logical_tos = 0;
static int in_else[10];
int isnum ( char *string, long *num );

static int if_exec ( int argc, char **argv ) {
	long cond;
	if ( argc != 2 ) {
		printf ( "Syntax: if <condition>\n" );
		return -1;
	}
	
	if ( !isnum ( argv[1], &cond ) ) {
		printf ( "non-numeric condition\n" );
		return -1;
	}
	
	in_else[logical_tos+1] = 0;
	if ( logical_tos > branch_tos || branch_stack[branch_tos] == 0 ) {
		logical_tos++;
		return 0;
	}
	
	//printf ( "Condition is %ld\n", cond );
	logical_tos = ++branch_tos;
	branch_stack[branch_tos] = cond ? 1 : 0;
	return 0;
}

struct command if_command __command = {
	.name = "if",
	.exec = if_exec,
};

static int fi_exec ( int argc, char **argv ) {
	if ( argc != 1 ) {
		printf ( "Syntax: %s\n", argv[0] );
		return -1;
	}
	
	if ( logical_tos > branch_tos ) {
		logical_tos--;
		return 0;
	}
	
	if ( branch_tos )
		logical_tos = --branch_tos;
	else {
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

	//printf ( "logical_tos = %d, branch_tos = %d\n", logical_tos, branch_tos );
	if ( ( logical_tos == 0 ) || ( in_else[logical_tos] != 0 ) ) {
		printf ( "else without if\n" );
		return -1;
	}
	
	in_else[logical_tos] = 1;
	if ( logical_tos > branch_tos ) {
		return 0;
	}
		
	branch_stack[branch_tos] = !branch_stack[branch_tos];
	return 0;
}

struct command else_command __command = {
	.name = "else",
	.exec = else_exec,
};