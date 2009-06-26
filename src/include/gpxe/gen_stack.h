#ifndef _GPXE_GEN_STACK_H
#define _GPXE_GEN_STACK_H


#include <ctype.h>
#include <errno.h>

#include <lib.h>

struct generic_stack {
	void *ptr;
	int tos;
	size_t size;
};

void init_generic_stack ( struct generic_stack *stack, size_t size );
int push_generic_stack ( struct generic_stack *stack, void *str, int is_string );
int pop_generic_stack ( struct generic_stack *stack, void *ptr );
void free_generic_stack ( struct generic_stack *stack, int on_stack );

/* convenience macros */
#define TOP_GEN_STACK_INT( stack ) ( ( ( int * ) ( stack )->ptr )[( stack )->tos] )
#define ELEMENT_GEN_STACK_INT( stack, pos ) ( ( ( int * ) ( stack )->ptr )[pos] )
#define SIZE_GEN_STACK( stack ) ( stack->tos )

#endif