#ifndef _GPXE_GEN_STACK_H
#define _GPXE_GEN_STACK_H

#include <ctype.h>
#include <errno.h>

/** Macro to define tos variable */
#define COUNT( stack ) _##stack##_tos

/** Define a stack of the given name, type and size */
#define INIT_STACK( name, type, size ) \
	typeof ( type ) name[size]; \
	int COUNT ( name ) = -1;

/** Define an extern stack with the given name, type and soze */
#define EXTERN_INIT_STACK( name, type, size ) \
	extern typeof ( type ) name[size]; \
	extern int COUNT ( name ); 

/** Define a static stack with the given name, type and size */
#define STATIC_INIT_STACK( name, type, size ) \
	static typeof ( type ) name[size];	\
	static int COUNT ( name ) = -1;

/** Push a value onto the stack */
#define PUSH_STACK( stack, value ) do {				\
		COUNT ( stack ) += 1;						\
		stack[COUNT ( stack )] = value;				\
} while ( 0 );

/** Use strdup to allocate a string on the heap and push it onto he stack */
#define PUSH_STACK_STRING( stack, value ) do {				\
		COUNT ( stack ) += 1;						\
		stack[COUNT ( stack )] = strdup ( value );		\
} while ( 0 );

/** Pop a value off the stack */
#define POP_STACK( stack, value ) do {				\
	value = stack[COUNT ( stack )];					\
	COUNT ( stack ) -= 1;							\
} while ( 0 );

#define DUP_STACK( array, stack, count ) do {				\
	memcpy ( array, stack, ( COUNT ( stack ) + 1 ) * sizeof ( typeof ( stack[0] ) ) );	\
	count = COUNT ( stack );	\
} while ( 0 );

/** Empty the stack */
#define FREE_STACK( stack ) do {						\
	COUNT ( stack ) = -1;							\
} while ( 0 );

/** Call free() on each element in the stack */
#define FREE_STACK_STRING( stack ) do {				\
	int i;											\
	for ( i = 0; i <= COUNT ( stack ); i++ )				\
		free ( stack[i] );							\
	FREE_STACK ( stack );							\
} while ( 0 );

#endif