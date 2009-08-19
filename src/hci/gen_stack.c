#include <stdlib.h>
#include <gpxe/gen_stack.h>

/** @file
 *
 * Generic stack
 *
 */

/**
 * Push an element onto the stack.
 *
 * @v stack	Stack to push
 * @v data_len	Size of data
 * @ret element	Pointer to be allocated memory on top of stack
 *
 * This function allocates memory on top of a given stack and returns a pointer
 * to it.
 */
void * stack_push_ ( struct stack *stack, size_t data_len ) {
	struct stack_element *element;
	
	element = malloc ( sizeof ( *element ) + data_len );
	if ( ! element )
		return NULL;
	
	list_add ( &(element->list), &stack->list );
	return &element->data;
}

/**
 * Return a pointer to the struct stack_element at the top of the stack
 *
 * @v stack	The stack
 * @ret element	Pointer to struct stack_element
 */
static struct stack_element * stack_top_element ( struct stack *stack ) {
	struct stack_element *element;
	struct list_head *list;
	
	if ( list_empty ( &stack->list ) )
		return NULL;
	
	list = stack->list.next;
	element = container_of ( list, struct stack_element, list );
	return element;
}

/**
 * Get a pointer to the topmost element of a stack
 * @v stack	Stack
 * @ret data	Pointer to the topmost element of the stack
 *
 * This function returns a pointer to the (data in the) topmost element of a
 * given stack
 */
void * stack_top ( struct stack *stack ) {
	struct stack_element *element = stack_top_element ( stack );
	if ( element )
		return &element->data;
	return NULL;
}

/** Remove the topmost element of a stack
 *
 * @v stack	Stack
 */
void stack_pop ( struct stack *stack ) {
	struct stack_element *element;
	
	element = stack_top_element ( stack );
	if ( element ) {
		list_del ( &element->list );
		free ( element );
	}
}

/**
 * Return a pointer to the struct stack_element at a given index.
 *
 * @v stack	Pointer to the struct stack
 * @v pos	Index of the element
 * @ret element	Pointer to the struct stack_element
 */
struct stack_element * stack_element_at ( struct stack *stack, int pos ) {
	int i = 0;
	struct stack_element *element;
	
	stack_for_each ( element, stack ) {
		if ( i++ == pos )
			return element;
	}
	return NULL;
}

/**
 * Get a pointer to data at the given position on the stack
 *
 * @v stack	Stack
 * @v pos	Index of the element
 * @ret data	Pointer to the data
 *
 * This function allows the user to use the stack like an array. It returns a
 * pointer to data at a given index in the given stack.
 */
void * stack_data_element_at ( struct stack *stack, int pos ) {
	struct stack_element *element = stack_element_at ( stack, pos );
	if ( element )
		return element->data;
	return NULL;
}

/**
 * Find the number of elements on a given stack
 *
 * @v stack	Pointer to the struct stack
 * @ret size	Number of elements on the stack
 */
int stack_size ( struct stack *stack ) {
	struct stack_element *cur;
	int size = 0;
	stack_for_each ( cur, stack )
		size++;
	return size;
}
