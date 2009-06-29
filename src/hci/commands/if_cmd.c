#include <stdio.h>
#include <unistd.h>
#include <gpxe/command.h>
#include <gpxe/gen_stack.h>
#include <gpxe/init.h>
#include <gpxe/settings.h>

struct generic_stack if_stack = { .ptr = NULL, .tos = -1, .size = sizeof ( int ) };
static struct generic_stack else_stack = { .ptr = NULL, .tos = -1, .size = sizeof ( int ) };
int if_tos = 0;
struct generic_stack if_stack;
static struct generic_stack else_stack;
static struct generic_stack loop_stack;
struct generic_stack command_list;
int prog_ctr;
int isnum ( char *string, long *num );

int system ( const char * );

struct while_info {
	int loop_start;
	int if_pos;
	int is_continue;
	int cur_arg;
};

static struct while_info for_info;

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
		free_generic_stack ( &if_stack, 0 );
		free_generic_stack ( &else_stack, 0 );
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
	init_generic_stack ( &loop_stack, sizeof ( struct while_info ) );
	init_generic_stack ( &command_list, sizeof ( char * ) );
	prog_ctr = 0;
	push_generic_stack ( &if_stack, &one, 0 );
	push_generic_stack ( &else_stack, &one, 0 );
	return;
}

struct init_fn initialise_if __init_fn ( INIT_NORMAL ) = {
	.initialise = init_if,
};

static int while_exec ( int argc, char **argv ) {
	struct while_info w;
	if ( argc != 2 ) {
		printf ( "Syntax: while <condition>\n" );
		return 1;
	}
	if ( if_exec ( argc, argv ) != 0 )
		return 1;
	TOP_GEN_STACK_INT ( &else_stack ) = 1;
	
	w.loop_start = prog_ctr;
	w.if_pos = if_stack.tos;
	w.is_continue = 0;
	w.cur_arg = 0;
	
	if ( push_generic_stack ( &loop_stack, &w, 0 ) )
		return 1;
	//printf ( "pc = %d. size of loop_stack = %d\n", prog_ctr, SIZE_GEN_STACK ( &loop_stack ) );
	return 0;
}

struct command while_command __command = {
	.name = "while",
	.exec = while_exec,
};

static int done_exec ( int argc, char **argv ) {
	int cond;
	int tmp_pc;
	int rc = 0;
	if ( argc != 1 ) {
		printf ( "Syntax: %s\n", argv[0] );
		return 1;
	}
	
	//printf ( "size of if_stack = %d. size of loop stack = %d\n", SIZE_GEN_STACK ( &if_stack ), SIZE_GEN_STACK ( &loop_stack ) );
	
	if ( SIZE_GEN_STACK ( &loop_stack ) == 0 ) {
		printf ( "done outside a loop\n" );
		return 1;
	}
	
	if ( pop_generic_stack ( &if_stack, &cond ) || pop_generic_stack ( &loop_stack, &for_info ) )
		return 1;
	
	while ( cond || for_info.is_continue ) {		
		tmp_pc = prog_ctr;
		prog_ctr = for_info.loop_start;
		
		while ( prog_ctr < tmp_pc ) {
			if ( ( rc = system ( ELEMENT_GEN_STACK_STRING ( &command_list, prog_ctr ) ) ) ) 
				return rc;
		}
		if ( pop_generic_stack ( &if_stack, &cond ) || pop_generic_stack ( &loop_stack, &for_info ) )
			return 1;
	}
	for_info.cur_arg = 0;
	return rc;
}

struct command done_command __command = {
	.name = "done",
	.exec = done_exec,
};

static int break_exec ( int argc, char **argv ) {
	int pos;
	struct while_info *w;
	if ( argc != 1 ) {
		printf ( "Syntax: %s\n", argv[0] );
		return 1;
	}
	if ( SIZE_GEN_STACK ( &loop_stack ) <= 0 ) {
		printf ( "%s outside loop\n", argv[0] );
	}
	w = ( ( struct while_info * ) loop_stack.ptr + ( loop_stack.tos ) );
	for ( pos = w->if_pos; pos < SIZE_GEN_STACK ( &if_stack ); pos++ )
		ELEMENT_GEN_STACK_INT ( &if_stack, pos ) = 0;
	return 0;
}

struct command break_command __command = {
	.name = "break",
	.exec = break_exec,
};

static int continue_exec ( int argc, char **argv ) {
	struct while_info *w;
	if ( argc != 1 ) {
		printf ( "Syntax: %s\n", argv[0] );
		return 1;
	}
	if ( break_exec ( argc, argv ) )
		return 1;
	w = ( ( struct while_info * ) loop_stack.ptr + ( loop_stack.tos ) );
	w->is_continue = 1;
	return 0;
}

struct command continue_command __command = {
	.name = "continue",
	.exec = continue_exec,
};

static int for_exec ( int argc, char **argv ) {
	int cond;
	int rc;
	if ( argc < 3 ) {
		printf ( "Syntax: for <var> in <list>\n" );
		return 1;
	}
	
	for_info.loop_start = prog_ctr;
	for_info.cur_arg = for_info.cur_arg == 0 ? 3 : for_info.cur_arg + 1;			//for_info should either be popped by a done or for_info.cur_arg = 0
	for_info.is_continue = 0;
	
	cond = TOP_GEN_STACK_INT ( &if_stack ) && ( argc > for_info.cur_arg );	
	if ( ( rc = push_generic_stack ( &if_stack, &cond, 0 ) ) == 0 )
		if ( ( rc = storef_named_setting ( argv[1], argv[for_info.cur_arg] ) ) == 0 ) 
			rc = push_generic_stack ( &loop_stack, &for_info, 0 );
	for_info.cur_arg = 0;
	return rc;
}

struct command for_command __command = {
	.name = "for",
	.exec = for_exec,
};
