#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <gpxe/command.h>
#include <gpxe/gen_stack.h>
#include <gpxe/init.h>
#include <gpxe/settings.h>
#include <gpxe/parse.h>
#include <hci/if_cmd.h>

static struct while_info for_info;

STACK ( if_stack );
STACK ( else_stack );
STACK ( loop_stack );
static int in_try;

size_t start_len;
size_t cur_len;

/**
 * Push a value onto the if stack
 *
 * @v condition		Value to push
 * @ret err		Non-zero if error occurs
 *
 * Both the if and else stacks need to be pushed at the same time.
 */
static int push_if ( int cond ) {
	int rc;
	int *if_pos;
	int *else_pos;
	
	/* cond is pushed only if the previous value is true */
	cond = * ( int * ) stack_top ( &if_stack ) && cond;
	/* Both the if and else stacks are to be pushed together */
	assert ( stack_size ( &if_stack ) == stack_size ( &else_stack ) );
	if_pos = stack_push ( &if_stack, int );
	if ( if_pos )
		*if_pos = cond;
	else_pos = stack_push ( &else_stack, int );
	if ( else_pos )
		*else_pos = 0;
	
	rc = !if_pos || !else_pos;
	return rc;
}

/**
 * Pop a value off the if stack
 *
 * @ret condition	Condition popped off the if stack
 * @ret rc		Non-zero if error occurs
 */
static int pop_if ( int *cond ) {
	int if_c;
	assert ( stack_size ( &if_stack ) == stack_size ( &else_stack ) );
	
	/* At all times, have at least one value on the if stack */
	if ( stack_size ( &if_stack ) > 0 ) {
		if_c = * ( int * ) stack_top ( &if_stack );
		stack_pop ( &if_stack );
		stack_pop ( &else_stack );
		if ( cond )
			*cond = if_c;
		return 0;
	}
	return 1;
}

/**
 * The "if" command
 */
static int if_exec ( int argc, char **argv ) {
	long cond;
	if ( argc != 2 ) {
		printf ( "Usage: if <condition>\n" );
		return 1;
	}
	
	if ( !isnum ( argv[1], &cond ) ) {
		printf ( "non-numeric condition: %s\n", argv[1] );
		return 1;
	}
	return push_if ( cond );
}

/** "if" command */
struct command if_command __command = {
	.name = "if",
	.exec = if_exec,
	.flags = 1,
};

/** The "fi" command
 */
static int fi_exec ( int argc, char **argv ) {
	int if_size;
	int loop_size;
	
	if ( argc != 1 ) {
		printf ( "Usage: %s\n", argv[0] );
		return 1;
	}
	if_size = stack_size ( &if_stack );
	loop_size = stack_size ( &loop_stack );
	/* if the if_stack is not empty and this is not a loop */	
	if ( if_size > 1 && ( loop_size == 0 ||
		( ( struct while_info * ) stack_top ( &loop_stack ) )->if_pos != if_size ) )
		return pop_if ( NULL );
	printf ( "fi without if\n" );
	return 1;
}

/** "fi" command */
struct command fi_command __command = {
	.name = "fi",
	.exec = fi_exec,
	.flags = 1,
};

/** The "else" command
*/
static int else_exec ( int argc, char **argv ) {
	if ( argc != 1 ) {
		printf ( "Usage: %s\n", argv[0] );
		return 1;
	}

	if ( * ( int * ) stack_top ( &else_stack ) != 0
		|| stack_size ( &if_stack ) <= 1 ) {

		printf ( "else without if\n" );
		return 1;
	}
	* ( int * ) stack_top ( &else_stack ) = 1;
	
	if ( element_at ( &if_stack, int, stack_size ( &if_stack ) - 2 ) )
		* ( int * ) stack_top ( &if_stack ) = ! * ( int * ) stack_top ( &if_stack );
	
	return 0;
}

/** "else" command */
struct command else_command __command = {
	.name = "else",
	.exec = else_exec,
	.flags = 1,
};

/** Reset the branch and loop stacks
 */
void init_if ( void ) {
	int *if_pos;
	int *else_pos;
	
	//free_stack ( &if_stack );
	//free_stack ( &else_stack );
	//free_stack ( &loop_stack );
	if_pos = stack_push ( &if_stack, int );
	else_pos = stack_push ( &else_stack, int );
	if ( if_pos )
		*if_pos = 1;
	if ( else_pos )
		*else_pos = 0;
	for_info.cur_arg = 0;
	return;
}

struct init_fn initialise_if __init_fn ( INIT_NORMAL ) = {
	.initialise = init_if,
};

/** The "while" command
 */
static int while_exec ( int argc, char **argv ) {
	struct while_info w;
	struct while_info *w_pos;
	
	if ( argc != 2 ) {
		printf ( "Usage: while <condition>\n" );
		return 1;
	}
	if ( if_exec ( argc, argv ) != 0 )
		return 1;
	* ( int * ) stack_top ( &else_stack ) = 1;
	
	w.loop_start = start_len;
	w.if_pos = stack_size ( &if_stack ) - 1;
	w.is_continue = 0;
	w.cur_arg = 0;
	w.is_catch = 0;
	w_pos = stack_push ( &loop_stack, struct while_info );
	if ( w_pos )
		*w_pos = w;
	return ( w_pos == NULL );
}

/** "while" command */
struct command while_command __command = {
	.name = "while",
	.exec = while_exec,
	.flags = 1,
};

/** The "done" command
 */
static int done_exec ( int argc, char **argv ) {
	int cond;
	int rc = 0;
	
	if ( argc != 1 ) {
		printf ( "Usage: %s\n", argv[0] );
		return 1;
	}
	
	if ( stack_size ( &loop_stack ) <= 0 ) {
		printf ( "done outside a loop\n" );
		return 1;
	}
	for_info = * ( struct while_info * ) stack_top ( &loop_stack );
	stack_pop ( &loop_stack );
	if ( pop_if ( &cond ) )
		return 1;
	if ( for_info.is_catch ) {
		cond = 0;
	}
	if ( cond || for_info.is_continue ) {
		cur_len = start_len = for_info.loop_start;
	} else
		for_info.cur_arg = 0;
	return rc;
}

/** "done" command */
struct command done_command __command = {
	.name = "done",
	.exec = done_exec,
	.flags = 1,
};

/** The "break" command
 */
static int break_exec ( int argc, char **argv ) {
	int pos;
	int start;
	if ( argc != 1 ) {
		printf ( "Usage: %s\n", argv[0] );
		return 1;
	}
	if ( stack_size ( &loop_stack ) <= 0 ) {
		printf ( "%s outside loop\n", argv[0] );
	}
	start = ( ( struct while_info * ) stack_top ( &loop_stack ) )->if_pos;
	for ( pos = start; pos < stack_size ( &if_stack ); pos++ )
		*element_at ( &if_stack, int, pos ) = 0;
	return 0;
}

/* "break" command */
struct command break_command __command = {
	.name = "break",
	.exec = break_exec,
	.flags = 0,
};

/**
 * The "continue" command
 */
static int continue_exec ( int argc, char **argv ) {
	struct while_info *w;
	if ( break_exec ( argc, argv ) )
		return 1;
	w = stack_top ( &loop_stack );
	w->is_continue = 1;
	return 0;
}

/** "continue command */
struct command continue_command __command = {
	.name = "continue",
	.exec = continue_exec,
};

/**
 * The "for" command
 */
static int for_exec ( int argc, char **argv ) {
	int cond;
	int rc = 0;
	struct while_info *loop_pos;
	
	if ( argc < 3 ) {
		printf ( "Usage: for <var> in <list>\n" );
		return 1;
	}
	
	for_info.loop_start = start_len;
	for_info.cur_arg = for_info.cur_arg == 0 ? 3 : for_info.cur_arg + 1;
	/* for_info should either be popped by a done or
	for_info.cur_arg = 0 */
	
	for_info.is_continue = 0;
		
	cond = * ( int * ) stack_top ( &if_stack ) && ( argc > for_info.cur_arg );
	
	if ( ( rc = push_if ( cond ) ) == 0 ) {
		for_info.if_pos = stack_size ( &if_stack ) - 1;
		DBG ( "setting %s to %s\n", argv[1], argv[for_info.cur_arg] );
		if ( ( rc = storef_named_setting ( argv[1],
				argv[for_info.cur_arg] ) ) == 0 ) {
			
			loop_pos = stack_push ( &loop_stack, struct while_info );
			if ( loop_pos )
				*loop_pos = for_info;
			rc = ( loop_pos == NULL );
		}
	}
	for_info.cur_arg = 0;
	return rc;
}

/** "for" command */
struct command for_command __command = {
	.name = "for",
	.exec = for_exec,
	.flags = 1,
};

/**
 * The "do" command
 */
static int do_exec ( int argc, char **argv ) {
	if ( argc != 1 ) {
		printf ( "Usage: %s\n", argv[0] );
		return 1;
	}
	/* This is just a nop, to make the syntax similar to the shell */
	return 0;
}

/*
 * "do" command
 */
struct command do_command __command = {
	.name = "do",
	.exec = do_exec,
	.flags = 0,
};

/**
 * The "try" command
 */
static int try_exec ( int argc, char **argv ) {
	if ( argc != 1 ) {
		printf ( "Usage: %s\n", argv[0] );
		return 1;
	}
	asprintf ( &argv[1], "%d", 1 );
	while_exec ( 2, argv );
	( ( struct while_info * ) stack_top ( &loop_stack ) )->is_catch = 1;
	* ( int * ) stack_top ( &else_stack ) = 0;
	in_try++;
	return 0;
}

/** "try" command */
struct command try_command __command = {
	.name = "try",
	.exec = try_exec,
	.flags = 1,
};

/*
 * The "catch" command
 */
static int catch_exec ( int argc, char **argv ) {
	/* Just an else statement with some extras */
	if ( else_exec ( argc, argv ) )
		return 1;
	in_try--;
	( ( struct while_info * ) stack_top ( &loop_stack ) )->is_continue = 0;
	return 0;
}

/** "catch" command */
struct command catch_command __command = {
	.name = "catch",
	.exec = catch_exec,
	.flags = 1,
};

void store_rc ( int rc ) {
	char *rc_string;
	asprintf ( &rc_string, "%d", rc );
	if ( rc_string )
		storef_named_setting ( "rc", rc_string );
	free ( rc_string );
}

void test_try ( int rc ) {
	int i;
	struct stack_element *element;
	if ( rc && in_try > 0 ) {
		/* Failed statement inside try block */
		DBG ( "Exiting try block\n" );
		/* Unwinding */
		stack_for_each ( element, &loop_stack )
		{
			if ( ( ( struct while_info * )element->data )->is_catch )
				break;
		}
		
		i = ( ( struct while_info * )element->data )->if_pos;
		
		for ( ; i < stack_size ( &if_stack ); i++ )
			* element_at ( &if_stack, int, i ) = 0;
	}
}
