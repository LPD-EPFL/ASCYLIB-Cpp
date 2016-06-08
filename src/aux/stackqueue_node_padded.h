#ifndef _STACKQUEUE_NODE_PADDED_H_
#define _STACKQUEUE_NODE_PADDED_H_

#include"ssmem.h"

extern __thread ssmem_allocator_t* alloc;

template<typename T>
struct stackqueue_node_padded
{
	union {
		struct {
			T item;
			struct stackqueue_node_padded* next;
		};
		uint8_t padding[CACHE_LINE_SIZE];
	};
};

template<typename T>
stackqueue_node_padded<T> *allocate_stackqueue_node_padded(T item,
		stackqueue_node_padded<T> *next)
{
#if GC == 1
	stackqueue_node_padded<T>* node = (stackqueue_node_padded<T>*)
		ssmem_alloc(alloc, sizeof(*node));
#else
	stackqueue_node_padded<T>* node = malloc(size(*node));
#endif
	node->item = item;
	node->next = next;
#ifdef __tile__
	MEM_BARRIER;
#endif
	return node;
}

template<typename T>
void release_stackqueue_node_padded(stackqueue_node_padded<T>* node)
{
#if GC == 1
	ssmem_free(alloc, (void*) node);
#endif
}

#endif
