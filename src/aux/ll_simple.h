#ifndef _LL_SIMPLE_H_
#define _LL_SIMPLE_H_

#include<stdlib.h>
#include<malloc.h>

extern __thread ssmem_allocator_t* alloc;

template <typename K, typename V>
struct ll_simple {
	K key;
	V val;
	volatile struct ll_simple *next;
	uint8_t padding32[8];
};

template <typename K, typename V>
volatile ll_simple<K,V> *initialize_ll_simple(K key, V val,
		volatile ll_simple<K,V> *next)
{
	volatile ll_simple<K,V> *node;
	node = (volatile ll_simple<K,V> *)
		malloc(sizeof(ll_simple<K,V>));
	if (node == NULL) {
		perror("malloc @ initialize_ll_simple");
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
volatile ll_simple<K,V> *allocate_ll_simple(K key, V val,
		volatile ll_simple<K,V> *next)
{
	volatile ll_simple<K,V> *node;
#if GC == 1
	node = (volatile ll_simple<K,V> *)
		ssmem_alloc(alloc, sizeof(ll_simple<K,V>));
#else
	node = (volatile ll_simple<K,V> *) malloc(sizeof(ll_simple<K,V>));
#endif
	if (node == NULL) {
		perror("malloc @ allocate_ll_simple");
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
void ll_simple_release(volatile ll_simple<K,V> *node)
{
#if GC == 1
	ssmem_free(alloc, (void*) node);
#endif
}

template<typename K, typename V>
void ll_simple_delete(volatile ll_simple<K,V> *node)
{
	//DESTROY_LOCK( &(node->lock) );
#if GC == 1
	free( (void*)node );
#endif
}


#endif
