#ifndef _GPXE_GEN_STACK_H
#define _GPXE_GEN_STACK_H


#include <ctype.h>
#include <errno.h>

#if 0
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

#define COUNT( stack ) _##stack##_tos

#define INIT_STACK( stack, type ) \
	typeof ( type ) *stack = NULL;	\
	int COUNT ( stack ) = -1;
	
#define EXTERN_INIT_STACK( stack, type ) \
	extern typeof ( type ) *stack; \
	extern int COUNT ( stack ); 
	
#define STATIC_INIT_STACK( stack, type ) \
	static typeof ( type ) *stack = NULL;	\
	static int COUNT ( stack ) = -1;

#define PUSH_STACK( stack, value ) do {				\
	typeof ( stack ) _tmp_stack;						\
	_tmp_stack = realloc ( stack, ( COUNT ( stack ) + 2 ) * sizeof ( typeof ( *stack ) ) );	\
	if ( _tmp_stack ) {								\
		stack = _tmp_stack;							\
		COUNT ( stack ) += 1;						\
		stack[COUNT ( stack )] = value;				\
	} else {										\
		FREE_STACK ( stack );						\
		DBG ( "couldn't push onto stack\n" );			\
	}											\
} while ( 0 );

#define PUSH_STACK_STRING( stack, value ) do {				\
	typeof ( stack ) tmp_stack;						\
	tmp_stack = realloc ( stack, ( COUNT ( stack ) + 2 ) * sizeof ( typeof ( *stack ) ) );	\
	if ( tmp_stack ) {								\
		stack = tmp_stack;							\
		COUNT ( stack ) += 1;						\
		stack[COUNT ( stack )] = strdup ( value );		\
	} else										\
		FREE_STACK ( stack );						\
} while ( 0 );

#define POP_STACK( stack, value ) do {				\
	value = stack[COUNT ( stack )];					\
	COUNT ( stack ) -= 1;							\
	if ( COUNT ( stack ) < 0 )						\
		FREE_STACK ( stack );						\
} while ( 0 );

#define FREE_STACK( stack ) do {						\
	free ( stack );									\
	stack = NULL;									\
	COUNT ( stack ) = -1;							\
} while ( 0 );

#define FREE_STACK_STRING( stack ) do {				\
	int i;											\
	for ( i = 0; i <= COUNT ( stack ); i++ )				\
		free ( stack[i] );							\
	FREE_STACK ( stack );							\
} while ( 0 );


#endif