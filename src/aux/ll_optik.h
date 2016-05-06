#ifndef _LL_OPTIK_H_
#define _LL_OPTIK_H_

#include<stdio.h>
#include<stdlib.h>
extern "C" {
#include"lock_if.h"
}

extern __thread ssmem_allocator_t* alloc;

template<typename K, typename V>
struct ll_optik
{
	K key;                   /* 8 */
	V val;                   /* 16 */
	volatile struct ll_optik<K,V> *next; /* 24 */
	volatile optik_t lock;       /* 32 */
};

template<typename K, typename V>
volatile ll_optik<K,V>* allocate_ll_optik(K key, V value,
		volatile ll_optik<K,V> *next)
{
	volatile ll_optik<K,V> *new_node;
#if GC==1
	new_node = (volatile ll_optik<K,V> *)
		ssmem_alloc(alloc, sizeof(ll_optik<K,V>));
#else
	new_node = (volatile ll_optik<K,V> *)
		malloc(sizeof(ll_optik<K,V>));
#endif
	if (new_node==NULL) {
		perror("malloc @ allocate_ll_marked");
		exit(1);
	}
	new_node->key = key;
	new_node->val = value;
	new_node->next = next;

	optik_init(&new_node->lock);

#if defined(__tile__)
	MEM_BARRIER;
#endif
	return new_node;
}

template<typename K, typename V>
volatile ll_optik<K,V>* initialize_ll_optik(
		K key, V value, volatile ll_optik<K,V> *next)
{
	volatile ll_optik<K,V> *new_node;
	new_node = (volatile ll_optik<K,V> *) malloc(sizeof(ll_optik<K,V>));
	if (new_node==NULL) {
		perror("malloc @ initialize_ll_optik");
		exit(1);
	}
	new_node->key = key;
	new_node->val = value;
	new_node->next = next;

	optik_init(&new_node->lock);

#if defined(__tile__)
	MEM_BARRIER;
#endif
	return new_node;
}

template<typename K, typename V>
void ll_optik_release(volatile ll_optik<K,V> *node)
{
#if GC == 1
	ssmem_free(alloc, (void*) node);
#endif
}

template<typename K, typename V>
void ll_optik_delete(volatile ll_optik<K,V> *node)
{
	DESTROY_LOCK( &(node->lock) );
#if GC == 1
	free( (void*)node );
#endif
}

#endif
