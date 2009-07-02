#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <gpxe/command.h>
#include <gpxe/gen_stack.h>
#include <gpxe/init.h>
#include <gpxe/settings.h>

int isnum ( char *string, long *num );

int system ( const char * );

struct while_info {
	struct command_entry *loop_start;
	int if_pos;
	int is_continue;
	int cur_arg;
};

static struct while_info for_info;
struct command_entry start_command = {
	.line[0] = 0,
	.neighbours = {
		.prev = &( start_command.neighbours ),
		.next = &( start_command.neighbours ),
	},
};

INIT_STACK ( if_stack, int, 10 );
STATIC_INIT_STACK ( else_stack, int, 10 );
INIT_STACK ( loop_stack, struct while_info, 10 );
struct command_entry *cur_command = &start_command;

static int push_if ( int cond ) {
	int rc;
	cond = if_stack[COUNT ( if_stack ) ] && cond;
	assert ( COUNT ( if_stack ) == COUNT ( else_stack ) );
	PUSH_STACK ( if_stack, cond );
	PUSH_STACK ( else_stack, 0 );
	rc = COUNT ( if_stack ) < 0 || COUNT ( else_stack ) < 0;
	DBG ( "if_stack size = %d, else_stack size = %d. rc = %d\n", COUNT ( if_stack ), COUNT ( else_stack ), rc );
	return rc;
}

static int pop_if ( int *cond ) {
	int else_c, if_c;
	assert ( COUNT ( if_stack ) == COUNT ( else_stack ) );
	if ( COUNT ( if_stack ) > 0 ) {
		POP_STACK ( if_stack, if_c );
		POP_STACK ( else_stack, else_c );
		if ( cond )
			*cond = if_c;
		return 0;
	}
	return 1;	
}

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

struct command if_command __command = {
	.name = "if",
	.exec = if_exec,
};

static int fi_exec ( int argc, char **argv ) {
	if ( argc != 1 ) {
		printf ( "Usage: %s\n", argv[0] );
		return 1;
	}
	
	if ( COUNT ( if_stack ) > 0 && ( COUNT ( loop_stack ) < 0 || COUNT ( if_stack ) != loop_stack[COUNT ( loop_stack )].if_pos ) )
		return pop_if ( NULL );
	printf ( "fi without if\n" );
	return 1;
}

struct command fi_command __command = {
	.name = "fi",
	.exec = fi_exec,
};

static int else_exec ( int argc, char **argv ) {
	if ( argc != 1 ) {
		printf ( "Usage: %s\n", argv[0] );
		return 1;
	}

	if ( else_stack[COUNT ( else_stack ) ] != 0 || COUNT ( if_stack ) <= 0 ) {
		printf ( "else without if\n" );
		return 1;
	}
	else_stack[COUNT ( else_stack ) ] = 1;
	
	if ( if_stack[COUNT ( if_stack ) - 1] )
		if_stack[COUNT ( if_stack ) ] = ! if_stack[COUNT ( if_stack ) ];
	
	return 0;
}

struct command else_command __command = {
	.name = "else",
	.exec = else_exec,
};

void init_if ( void ) {
	PUSH_STACK ( if_stack, 1 );
	PUSH_STACK ( else_stack, 1 );
	return;
}

struct init_fn initialise_if __init_fn ( INIT_NORMAL ) = {
	.initialise = init_if,
};

static int while_exec ( int argc, char **argv ) {
	struct while_info w;
	if ( argc != 2 ) {
		printf ( "Usage: while <condition>\n" );
		return 1;
	}
	if ( if_exec ( argc, argv ) != 0 )
		return 1;
	else_stack[COUNT ( else_stack )] = 1;
	
	w.loop_start = cur_command;
	DBG ( "pushing [%s]\n", cur_command->line );
	w.if_pos = COUNT ( if_stack );
	w.is_continue = 0;
	w.cur_arg = 0;
	PUSH_STACK ( loop_stack, w );
	return ( loop_stack == NULL );
}

struct command while_command __command = {
	.name = "while",
	.exec = while_exec,
};

static int done_exec ( int argc, char **argv ) {
	int cond;
	struct command_entry *tmp_cmd;
	//int tmp_pc;
	int rc = 0;
	if ( argc != 1 ) {
		printf ( "Usage: %s\n", argv[0] );
		return 1;
	}
	
	if ( COUNT ( loop_stack ) < 0 ) {
		printf ( "done outside a loop\n" );
		return 1;
	}
	POP_STACK ( loop_stack, for_info );
	if ( pop_if ( &cond ) )
		return 1;
	
	while ( cond || for_info.is_continue ) {
		tmp_cmd = cur_command;
		cur_command = for_info.loop_start;
		
		do {
			if ( ( rc = system ( cur_command->line ) ) ) {
				DBG ( "bailing out at [%s]\n", cur_command->line );
				break;
			}
			if ( cur_command == &start_command ) {
				DBG ( "reached starting while looking for done\n" );
				rc = 1;
				break;
			}
		} while ( cur_command != tmp_cmd );
		cur_command = tmp_cmd;
		if ( rc != 0 )
			return rc;
		POP_STACK ( loop_stack, for_info );
		if ( pop_if ( &cond ) )
			return 1;
	}
	DBG ( "exited loop. stack size = %d\n", COUNT ( loop_stack ) + 1 );
	for_info.cur_arg = 0;
	return rc;
}

struct command done_command __command = {
	.name = "done",
	.exec = done_exec,
};

static int break_exec ( int argc, char **argv ) {
	int pos;
	int start;
	if ( argc != 1 ) {
		printf ( "Usage: %s\n", argv[0] );
		return 1;
	}
	if ( COUNT ( loop_stack ) < 0 ) {
		printf ( "%s outside loop\n", argv[0] );
	}
	start = loop_stack[COUNT ( loop_stack )].if_pos;
	for ( pos = start; pos <= COUNT ( if_stack ); pos++ )
		if_stack[pos] = 0;
	return 0;
}

struct command break_command __command = {
	.name = "break",
	.exec = break_exec,
};

static int continue_exec ( int argc, char **argv ) {
	struct while_info *w;
	if ( break_exec ( argc, argv ) )
		return 1;
	w = loop_stack + COUNT ( loop_stack );
	w->is_continue = 1;
	return 0;
}

struct command continue_command __command = {
	.name = "continue",
	.exec = continue_exec,
};

static int for_exec ( int argc, char **argv ) {
	int cond;
	int rc = 0;
	if ( argc < 3 ) {
		printf ( "Usage: for <var> in <list>\n" );
		return 1;
	}
	
	for_info.loop_start = cur_command;
	DBG ( "pushing [%s]\n", cur_command->line );
	for_info.cur_arg = for_info.cur_arg == 0 ? 3 : for_info.cur_arg + 1;			//for_info should either be popped by a done or for_info.cur_arg = 0
	for_info.is_continue = 0;
	
	cond = if_stack[COUNT ( if_stack ) ] && ( argc > for_info.cur_arg );
	if ( ( rc = push_if ( cond ) ) == 0 ) {
		for_info.if_pos = COUNT ( if_stack );
		DBG ( "setting %s to %s\n", argv[1], argv[for_info.cur_arg] );
		if ( ( rc = storef_named_setting ( argv[1], argv[for_info.cur_arg] ) ) == 0 ) {
			PUSH_STACK ( loop_stack, for_info );
			rc = ( loop_stack == NULL );
		}
	}
	for_info.cur_arg = 0;
	return rc;
}

struct command for_command __command = {
	.name = "for",
	.exec = for_exec,
};

static int do_exec ( int argc, char **argv ) {
	if ( argc != 1 ) {
		printf ( "Usage: %s\n", argv[0] );
		return 1;
	}
	/* This is just a nop, to make the syntax similar to the shell */
	return 0;
}

struct command do_command __command = {
	.name = "do",
	.exec = do_exec,
};

void free_command_list () {
	struct command_entry *cur = &start_command;
	cur = list_entry ( cur->neighbours.next, struct command_entry, neighbours );
	while ( cur != &start_command ) {
		struct command_entry *temp;
		temp = list_entry ( cur->neighbours.next, struct command_entry, neighbours );
		free ( cur );
		cur = temp;
	}
	INIT_LIST_HEAD ( &start_command.neighbours );
	cur_command = &start_command;
}