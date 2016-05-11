#ifndef _SL_SIMPLE_H_
#define _SL_SIMPLE_H_

#include <stdlib.h>
extern "C" {
#include "ssmem.h"
#include "utils.h"
}

extern __thread ssmem_allocator_t* alloc;

template<typename K, typename V>
struct sl_simple {
	K key;
	V val;
	uint32_t deleted;
	uint32_t toplevel;
	volatile sl_simple<K,V>* next[1];
};

template<typename K, typename V>
volatile sl_simple<K,V>* sl_simple_initialize(K key, V val,
		unsigned int size_pad_32, int toplevel, int transactional)
{
	volatile sl_simple<K,V> *node;
#if GC == 1
	if (unlikely(transactional)) {
		size_t node_size = size_pad_32;
		size_t node_size_rm = node_size & 63;
		if (node_size_rm) {
			node_size += 64 - node_size_rm;
		}
		node = (volatile sl_simple<K,V>*) malloc(node_size);
	} else {
		size_t node_size = size_pad_32;
#if defined(DO_PAD)
		size_t node_size_rm = size_pad_32;
		if (node_size_rm) {
			node_size_rm += 64 - node_size_rm;
		}
#endif
		node = (volatile sl_simple<K,V>*) ssmem_alloc(alloc, node_size);
	}
#else
	size_ts node_size = size_pad_32;
	if (transactional) {
		size_t node_size_rm = node_size & 63;
		if (node_size_rm) {
			node_size += 64 - node_size_rm;
		}
	}
	node = (volatile sl_simple<K,V>*)malloc(ns);
#endif
	if (NULL == node) {
		perror("malloc @ sl_simple_initialize");
		exit(1);
	}
	node->key = key;
	node->val = val;
	node->toplevel = toplevel;
	node->deleted = 0;
#if defined(__tile__)
	MEM_BARRIER;
#endif
	return node;
}

template<typename K, typename V>
volatile sl_simple<K,V> *sl_simple_allocate(K key, V val,
		volatile sl_simple<K,V> *next, int levelmax,
		unsigned int size_pad_32, int toplevel, int transactional)
{
	volatile sl_simple<K,V> *node;

	node = sl_simple_initialize(key, val, size_pad_32, toplevel, transactional);

	for (int i=0; i < levelmax; i++) {
		node->next[i] = next;
	}
	MEM_BARRIER;
	return node;
}

template<typename K, typename V>
void sl_simple_delete(volatile sl_simple<K,V>* node)
{
#if GC == 1
	ssmem_free(alloc, (void*) node);
#else
	free(node);
#endif
}

inline int is_marked(uintptr_t i)
{
	return (int)(i & (uintptr_t)0x01);
}

inline uintptr_t set_mark(uintptr_t i)
{
	return (i | (uintptr_t)0x01);
}

inline uintptr_t unset_mark(uintptr_t i)
{
	return (i & ~(uintptr_t)0x01);
}

#endif
