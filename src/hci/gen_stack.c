#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <gpxe/gen_stack.h>



void init_generic_stack ( struct generic_stack *stack, size_t size ) {
	stack->ptr = NULL;
	stack->tos = -1;
	stack->size = size;
}


int pop_generic_stack ( struct generic_stack *stack, void *ptr ) {
	if ( stack->tos >= 0 ) {
		void *nptr;
		memcpy ( ptr, ( void * ) ( ( (int)stack->ptr ) + stack->size * stack->tos-- ), stack->size );
		nptr = realloc ( stack->ptr, stack->size * ( stack->tos + 1 ) );
		if ( nptr ) {
			stack->ptr = nptr;
			if ( stack->tos == -1 )
				stack->ptr = NULL;
			return 0;
		} else
			return -ENOMEM;
	} else
		return -ENOMEM;
}

void free_generic_stack ( struct generic_stack *stack, int on_stack ) {
	void *ptr = NULL;
	if ( on_stack ) {
		while ( !pop_generic_stack ( stack, ptr ) ) {
			free ( ptr );
		}
	}
	free ( stack->ptr );
}

int push_generic_stack ( struct generic_stack *stack, void *str, int is_string ) {
	char **nptr;
	nptr = realloc ( stack->ptr, stack->size * ( stack->tos + 2 ) );
	if ( !nptr ) {
		printf ( "error in resizing stack\n" );
		return -ENOMEM;
	}
	stack->ptr = nptr;
	stack->tos++;
	if ( !is_string ) 
		memcpy ( ( void * ) ( ( ( int ) stack->ptr ) + stack->size * stack->tos ), str, stack->size );
	else {
		if ( asprintf ( ( char ** ) ( ( ( int ) stack->ptr ) + stack->size * stack->tos ), "%s", *( char ** ) str ) < 0 ) {
			return -ENOMEM;
		}
	}
	return 0;
}
