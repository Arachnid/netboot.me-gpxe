#ifndef _GPXE_GEN_STACK_H
#define _GPXE_GEN_STACK_H


#include <ctype.h>
#include <errno.h>

struct generic_stack {
	void *ptr;
	int tos;
};

void init_generic_stack ( struct generic_stack *stack );
int push_generic_stack_ ( struct generic_stack *stack, void *str, int is_string, size_t size );	/* Use is_string = 1 to allocate a new string on the heap */
int pop_generic_stack_ ( struct generic_stack *stack, void *ptr, size_t size );
void free_generic_stack ( struct generic_stack *stack, int on_stack, size_t size );			/* Use on_stack = 1 to free stack values on the heap */

#define  push_generic_stack( stack, ptr, is_string ) push_generic_stack_ ( stack, ptr, is_string, sizeof ( typeof ( *ptr ) ) )

#define pop_generic_stack( stack, ptr ) pop_generic_stack_ ( stack, ptr, sizeof ( *ptr ) )


/* convenience macros */
#define TOP_GEN_STACK_INT( stack ) ( ( ( int * ) ( stack )->ptr )[( stack )->tos] )
#define ELEMENT_GEN_STACK_INT( stack, pos ) ( ( ( int * ) ( stack )->ptr )[pos] )
#define SIZE_GEN_STACK( stack ) ( ( ( stack ) ->tos ) + 1 )

#define TOP_GEN_STACK_STRING( stack ) ( ( ( char ** ) ( stack )->ptr )[( stack )->tos] )
#define ELEMENT_GEN_STACK_STRING( stack, pos ) ( ( ( char ** ) ( stack )->ptr )[pos] )

#endif