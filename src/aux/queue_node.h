#ifndef _QUEUE_NODE_H_
#define _QUEUE_NODE_H_

#include"ssmem.h"

extern __thread ssmem_allocator_t* alloc;

template<typename T>
struct queue_node
{
	T item;
	struct queue_node* next;
};

template<typename T>
queue_node<T> *allocate_queue_node(T item, queue_node<T> *next)
{
#if GC == 1
	queue_node<T>* node = (queue_node<T>*)ssmem_alloc(alloc, sizeof(*node));
#else
	queue_node<T>* node = malloc(size(*node));
#endif
	node->item = item;
	node->next = next;
#ifdef __tile__
	MEM_BARRIER;
#endif
	return node;
}

template<typename T>
void release_queue_node(queue_node<T>* node)
{
#if GC == 1
	ssmem_free(alloc, (void*) node);
#endif
}

#endif
