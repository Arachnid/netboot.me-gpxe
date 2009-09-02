#ifndef _HCI_IF_CMD
#define _HCI_IF_CMD

#include <gpxe/gen_stack.h>

#define IF_SIZE 10

extern struct stack if_stack;

extern size_t start_len;
extern size_t cur_len;

void init_if ( void );

struct while_info {
	int loop_start;
	int if_pos;
	int is_continue;
	int is_catch;
	int cur_arg;
};


#endif

