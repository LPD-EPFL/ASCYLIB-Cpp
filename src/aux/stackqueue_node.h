#ifndef _STACKQUEUE_NODE_H_
#define _STACKQUEUE_NODE_H_

#include"ssmem.h"

extern __thread ssmem_allocator_t* alloc;

template<typename T>
struct stackqueue_node
{
	T item;
	struct stackqueue_node* next;
};

template<typename T>
stackqueue_node<T> *allocate_stackqueue_node(T item, stackqueue_node<T> *next)
{
#if GC == 1
	stackqueue_node<T>* node = (stackqueue_node<T>*)
		ssmem_alloc(alloc, sizeof(*node));
#else
	stackqueue_node<T>* node = malloc(size(*node));
#endif
	node->item = item;
	node->next = next;
#ifdef __tile__
	MEM_BARRIER;
#endif
	return node;
}

template<typename T>
void release_stackqueue_node(stackqueue_node<T>* node)
{
#if GC == 1
	ssmem_free(alloc, (void*) node);
#endif
}

#endif
