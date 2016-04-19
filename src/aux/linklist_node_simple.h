#ifndef __LINKEDLIST_NODE_SIMPLE_H__
#define __LINKEDLIST_NODE_SIMPLE_H__

#include<stdlib.h>
#include<malloc.h>

extern size_t array_ll_fixed_size;
template <typename K, typename V>
struct node_ll_simple {
	K key;
	V val;
};

template <typename K, typename V>
struct ll_array {
	size_t size;
	node_ll_simple<K,V> *nodes;
};

template <typename K, typename V>
inline volatile ll_array<K,V> *initialize_ll_array(size_t size)
{
	volatile ll_array<K,V> *all;
	all = (volatile ll_array<K,V> *) memalign( CACHE_LINE_SIZE,
			sizeof(ll_array<K,V>) +
			array_ll_fixed_size * sizeof(node_ll_simple<K,V>));
	assert(all != NULL);
	all->size = size;
	all->nodes = (node_ll_simple<K,V> *) (uintptr_t)all + sizeof(ll_array<K,V>);
	return all;
}

template <typename K, typename V>
inline volatile ll_array<K,V> *allocate_ll_array(size_t size)
{
	volatile ll_array<K,V> *all;
	all = (volatile ll_array<K,V> *) malloc(sizeof(ll_array<K,V>) +
		array_ll_fixed_size * sizeof(node_ll_simple<K,V>));
	assert(all != NULL);
	all->size = size;
	all->nodes = (node_ll_simple<K,V> *) (uintptr_t)all + sizeof(ll_array<K,V>);
	return all;
}
#endif
