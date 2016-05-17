#ifndef _SL_LOCKED_H_
#define _SL_LOCKED_H_

#include"optik.h"
#include<stdlib.h>
#include"ssmem.h"

template <typename K, typename V>
struct sl_locked {
	K key;
	V val;
	uint32_t toplevel;
	uint32_t state; // 0: being inserted / 1: valid / 2: deleted
	volatile optik_t lock;
	volatile struct sl_locked<K,V>* next[1];
};

template <typename K, typename V>
volatile sl_locked<K,V>* allocate_sl_locked_unlinked(K key, V val,
		unsigned int size_pad_32, int toplevel, int transactional)
{
	volatile sl_locked<K,V>* node;
#if GC == 1
	if (unlikely(transactional)) {
		// use levelmax instead of toplevel in order to be able to
		// use the ssalloc allocator
		size_t node_size = size_pad_32;
		size_t node_size_rm = node_size & 63;
		if (node_size_rm) {
			node_size_rm += 64 - node_size_rm;
		}
		node = (volatile sl_locked<K,V>*)malloc(node_size);
	} else {
		size_t node_size = size_pad_32;
#	if defined(DO_PAD)
		size_t node_size_rm = node_size & 63;
		if (node_size_rm) {
			node_size += 64 - node_size_rm;
		}
#	endif
		node = (volatile sl_locked<K,V>*) ssmem_alloc(alloc, node_size);
	}
#else
	size_t node_size = size_pad_32;
	if (transactional) {
		size_t node_size_rm = node_size & 63;
		if (node_size) {
			node_size += 64 - node_size_rm;
		}

	}
	node = (volatile sl_locked<K,V>*) malloc(node_size);
#endif
	node->key = key;
	node->val = val;
	node->toplevel = toplevel;
	node->state = 0;
	optik_init(&node->lock);
#if defined(__tile__)
	MEM_BARRIER;
#endif
	return node;
}

template <typename K, typename V>
volatile sl_locked<K,V>* allocate_sl_locked(K key, V val,
		volatile sl_locked<K,V> *next, int levelmax,
		unsigned int size_pad_32, int toplevel, int transactional)
{
	volatile sl_locked<K,V>* node;
	node = allocate_sl_locked_unlinked(key, val, size_pad_32,
			toplevel, transactional);
	for (int i=0; i < toplevel; i++) {
		node->next[i] = next;
	}
	MEM_BARRIER;

	return node;
}

template <typename K, typename V>
void delete_sl_locked(volatile sl_locked<K,V> *node)
{
	DESTROY_LOCK(ND_GET_LOCK(node));
	ssfree_alloc(1, (void*)node);
}

#endif
