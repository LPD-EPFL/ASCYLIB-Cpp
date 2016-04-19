#ifndef __LINKLIST_NODE_MARKED_H
#define __LINKLIST_NODE_MARKED_H

#include<stdio.h>
#include<stdlib.h>
#include<lock_if.h>

__thread ssmem_allocator_t* alloc;

template<typename K, typename V>
/* typedef volatile*/ struct node_ll_marked
{
	K key;                   /* 8 */
	V val;                   /* 16 */
	volatile struct node_ll_marked<K,V> *next; /* 24 */
	volatile uint8_t marked;
#if !defined(LL_GLOBAL_LOCK)
	volatile ptlock_t lock;       /* 32 */
#endif
#if defined(DO_PAD)
	uint8_t padding[CACHE_LINE_SIZE - sizeof(K) - sizeof(V) - sizeof(struct node*) - sizeof(uint8_t) - sizeof(ptlock_t)];
#endif
};

template <typename K, typename V>
struct node_ll_marked_cache
{
	K key;
	volatile node_ll_marked<K,V> *cached_node;
};

template<typename K, typename V>
volatile node_ll_marked<K,V>* allocate_new_marked_ll_node(K key, V value,
		volatile node_ll_marked<K,V> *next)
{
	volatile node_ll_marked<K,V> *new_node;
#if GC==1
	new_node = (volatile node_ll_marked<K,V> *)
		ssmem_alloc(alloc, sizeof(node_ll_marked<K,V>));
#else
	new_node = (volatile node_ll_marked<K,V> *)
		malloc(sizeof(node_ll_marked<K,V>));
#endif
	if (new_node==NULL) {
		perror("malloc @ allocate_new_marked_ll_node");
		exit(1);
	}
	new_node->key = key;
	new_node->val = value;
	new_node->next = next;
	new_node->marked = 0;

	INIT_LOCK(ND_GET_LOCK(new_node));

#if defined(__tile__)
	MEM_BARRIER;
#endif
	return new_node;
}

template<typename K, typename V>
volatile node_ll_marked<K,V>* initialize_new_marked_ll_node(K key, V value, volatile node_ll_marked<K,V> *next)
{
	volatile node_ll_marked<K,V> *new_node;
	new_node = (volatile node_ll_marked<K,V> *) malloc(sizeof(node_ll_marked<K,V>));
	if (new_node==NULL) {
		perror("malloc @ initialize_new_marked_ll_node");
		exit(1);
	}
	new_node->key = key;
	new_node->val = value;
	new_node->next = next;
	new_node->marked = 0;

	INIT_LOCK(ND_GET_LOCK(new_node));

#if defined(__tile__)
	MEM_BARRIER;
#endif
	return new_node;
}

template<typename K, typename V>
void node_ll_marked_release(volatile node_ll_marked<K,V> *node)
{
#if GC == 1
	ssmem_free(alloc, (void*) node);
#endif
}

template<typename K, typename V>
void node_ll_marked_delete(volatile node_ll_marked<K,V> *node)
{
	DESTROY_LOCK( &(node->lock) );
#if GC == 1
	free( (void*)node );
#endif
}

#endif
