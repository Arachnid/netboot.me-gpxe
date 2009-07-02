#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <gpxe/gen_stack.h>

#if 0
void init_generic_stack ( struct generic_stack *stack ) {
	stack->ptr = NULL;
	stack->tos = -1;
}

int pop_generic_stack_ ( struct generic_stack *stack, void *ptr, size_t size ) {
	if ( stack->tos >= 0 ) {
		void *nptr;
		memcpy ( ptr, stack->ptr + size * stack->tos--, size );
		if ( stack->tos == -1 ) {
			free ( stack->ptr );
			stack->ptr = NULL;
			return 0;
		}
		nptr = realloc ( stack->ptr, size * ( stack->tos + 1 ) );
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

void free_generic_stack ( struct generic_stack *stack, int on_stack, size_t size ) {
	void *ptr = NULL;
	if ( on_stack ) {
		while ( !pop_generic_stack_ ( stack, &ptr, size ) ) {
			free ( ptr );
		}
	}
	free ( stack->ptr );
	stack->ptr = NULL;
}

int push_generic_stack_ ( struct generic_stack *stack, void *str, int is_string, size_t size ) {
	char **nptr;
	nptr = realloc ( stack->ptr, size * ( stack->tos + 2 ) );
	if ( !nptr ) {
		printf ( "error in resizing stack\n" );
		return -ENOMEM;
	}
	stack->ptr = nptr;
	stack->tos++;
	if ( !is_string ) 
		memcpy ( stack->ptr + size * stack->tos, str, size );
	else {
		if ( ( TOP_GEN_STACK_STRING ( stack ) = strdup ( *( char ** ) str ) ) == NULL ) {
			return -ENOMEM;
		}
	}
	//printf ( "[%s] allocated\n", TOP_GEN_STACK_STRING ( stack ) );
	return 0;
}
#endif
