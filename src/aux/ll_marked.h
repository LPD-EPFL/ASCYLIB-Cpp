#ifndef _LL_MARKED_H_
#define _LL_MARKED_H_

#include<stdio.h>
#include<stdlib.h>
#include<lock_if.h>

extern __thread ssmem_allocator_t* alloc;

template<typename K, typename V>
struct ll_marked
{
	union {
		struct{
			K key;                   /* 8 */
			V val;                   /* 16 */
			volatile struct ll_marked<K,V> *next; /* 24 */
			volatile uint8_t marked;
#if !defined(LL_GLOBAL_LOCK)
			volatile ptlock_t lock;       /* 32 */
#endif
		};
		uint8_t padding[CACHE_LINE_SIZE];
	};
};

template <typename K, typename V>
struct ll_marked_cache
{
	K key;
	volatile ll_marked<K,V> *cached_node;
};

template<typename K, typename V>
volatile ll_marked<K,V>* allocate_ll_marked(K key, V value,
		volatile ll_marked<K,V> *next)
{
	volatile ll_marked<K,V> *new_node;
#if GC==1
	new_node = (volatile ll_marked<K,V> *)
		ssmem_alloc(alloc, sizeof(ll_marked<K,V>));
#else
	new_node = (volatile ll_marked<K,V> *)
		malloc(sizeof(ll_marked<K,V>));
#endif
	if (new_node==NULL) {
		perror("malloc @ allocate_ll_marked");
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
volatile ll_marked<K,V>* initialize_ll_marked(
		K key, V value, volatile ll_marked<K,V> *next)
{
	volatile ll_marked<K,V> *new_node;
	new_node = (volatile ll_marked<K,V> *) malloc(sizeof(ll_marked<K,V>));
	if (new_node==NULL) {
		perror("malloc @ initialize_ll_marked");
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
void ll_marked_release(volatile ll_marked<K,V> *node)
{
#if GC == 1
	ssmem_free(alloc, (void*) node);
#endif
}

template<typename K, typename V>
void ll_marked_delete(volatile ll_marked<K,V> *node)
{
	DESTROY_LOCK( &(node->lock) );
#if GC == 1
	free( (void*)node );
#endif
}

#endif
