#ifndef _GPXE_GEN_STACK_H
#define _GPXE_GEN_STACK_H

#include <gpxe/list.h>

struct stack {
	struct list_head list;
};

struct stack_element {
	struct list_head list;
	char data[0];
};
/**
 * Get a pointer to the data in the struct stack_element
 *
 * @v element	Pointer to the struct stack_element
 * @v type	Type of the data
 * @ret data	Pointer to the data, typecast to the correct type
 */
#define element_data( element, type )					\
	( ( type * ) element->data )

#define STACK( _name )							\
	struct stack _name = { LIST_HEAD_INIT ( _name.list ) }
#define EXTERN_STACK( _name )						\
	extern struct stack _name;

extern void * stack_push_ ( struct stack *stack, size_t data_len );
/**
 * Allocate memory on a stack
 *
 * @v stack	Pointer to the struct stack
 * @v type	Type of data
 *
 * This just allocates memory on top of the stack for data of a given type,
 * and returns a pointer to it. It is the caller's responsibility to check
 * that the pointer is valid, and copy the actual data.
 */
#define stack_push( stack, type )					\
	( ( type * ) stack_push_ ( stack, sizeof ( type ) ) )

extern void * stack_top ( struct stack *stack );
/**
 * Get a pointer to the element at the top of the stack
 *
 * @v stack	Pointer to the struct stack
 * @v type	Type of data
 */
#define element_at_top( stack, type )					\
	( ( type * ) stack_top ( stack ) )

/**
 * Remove the topmost entry in the stack
 *
 * @v stack	Pointer to the struct stack
 */
extern void stack_pop ( struct stack *stack );

extern struct stack_element * stack_element_at ( struct stack *stack, int pos );
extern void * stack_data_element_at ( struct stack *stack, int pos );
/**
 * Get a pointer to data at a given index in the stack
 *
 * @v stack	The stack
 * @v type	Type of data
 * @v pos	Index
 */
#define element_at( stack, type, pos )					\
	( ( type * ) stack_data_element_at ( stack, pos ) )

/**
 * Get number of elements on the stack
 *
 * @v stack	Pointer to the struct stack
 */
extern int stack_size ( struct stack *stack );

/**
 * Iterate over entries in a stack
 *
 * @v element	struct stack_element * to use as a loop counter
 * @v stack	Pointer to struct stack
 */
#define stack_for_each( element, stack )				\
	list_for_each_entry ( element, &( stack )->list, list )

/**
 * Iterate over entries in a stack, safe against deletion of entries
 *
 * @v element	struct stack_element * to use as a loop counter
 * @v temp	Another struct stack_element * for temporary storage
 * @v stack	The struct stack *
 */
#define stack_for_each_safe( element, temp, stack )			\
	list_for_each_entry_safe ( element, temp, &( stack )->list, list )

/**
 * Iterate over entries in a stack, starting from a given index
 *
 * @v element	struct stack_element * to use as a loop counter
 * @v start	Index to start from
 * @v stack	The struct stack *
 */
#define stack_each_from( element, start, stack )			\
	for ( element = stack_element_at ( stack, start );		\
		&element->list != &( stack )->list;			\
		element = list_entry ( element->list.next,		\
					struct stack_element,		\
					list ) )
/**
 * Deallocate a stack
 *
 * @v stack	Pointer to the struct stack
 */
#define free_stack( stack ) do {					\
	struct stack_element *_element, *_temp;				\
	stack_for_each_safe ( _element, _temp, stack ) {		\
		free ( _element );					\
	}								\
} while ( 0 )

/**
 * Deallocate a stack when all elements are struct strings
 * @v stack	Pointer to the struct stack
 */
#define free_stack_string( stack ) do {					\
	struct stack_element *_element, *_temp;				\
	stack_for_each_safe ( _element, _temp, stack ) {		\
		free_string ( ( struct string * ) _element->data );	\
		free ( _element );					\
	}								\
} while ( 0 )

#endif
