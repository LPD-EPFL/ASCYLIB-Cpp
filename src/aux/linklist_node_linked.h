#ifndef __LINKEDLIST_NODE_LINKED_H__
#define __LINKEDLIST_NODE_LINKED_H__

#include<stdlib.h>
#include<malloc.h>

extern __thread ssmem_allocator_t* alloc;

template <typename K, typename V>
struct node_ll_linked {
	K key;
	V val;
	volatile struct node_ll_linked *next;
	uint8_t padding32[8];
};

template <typename K, typename V>
volatile node_ll_linked<K,V> *initialize_node_ll_linked(K key, V val,
		volatile node_ll_linked<K,V> *next)
{
	volatile node_ll_linked<K,V> *node;
	node = (volatile node_ll_linked<K,V> *)
		malloc(sizeof(node_ll_linked<K,V>));
	if (node == NULL) {
		perror("malloc @ linkedlist_node_linked");
		exit(1);
	}
	node->key = key;
	node->val = val;
	node->next = next;
#if defined(__tile__)
	/* on tilera you may have store reordering causing the pointer to a new node
	 *      to become visible, before the contents of the node are visible */
	MEM_BARRIER;
#endif
	return node;
}

template <typename K, typename V>
volatile node_ll_linked<K,V> *allocate_node_ll_linked(K key, V val,
		volatile node_ll_linked<K,V> *next)
{
	volatile node_ll_linked<K,V> *node;
#if GC == 1
	node = (volatile node_ll_linked<K,V> *)
		ssmem_alloc(alloc, sizeof(node_ll_linked<K,V>));
#else
	node = (volatile node_ll_linked<K,V> *) malloc(sizeof(node_ll_linked<K,V>));
#endif
	if (node == NULL) {
		perror("malloc @ linkedlist_node_linked");
		exit(1);
	}
	node->key = key;
	node->val = val;
	node->next = next;
#if defined(__tile__)
	/* on tilera you may have store reordering causing the pointer to a new node
	 *      to become visible, before the contents of the node are visible */
	MEM_BARRIER;
#endif
	return node;
}

template<typename K, typename V>
void node_ll_linked_release(volatile node_ll_linked<K,V> *node)
{
#if GC == 1
	ssmem_free(alloc, (void*) node);
#endif
}

template<typename K, typename V>
void node_ll_linked_delete(volatile node_ll_linked<K,V> *node)
{
	DESTROY_LOCK( &(node->lock) );
#if GC == 1
	free( (void*)node );
#endif
}


#endif
