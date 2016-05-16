#ifndef _SL_MARKED_H_
#define _SL_MARKED_H_

template <typename K, typename V>
struct sl_marked {
	K key;
	V val;
	uint32_t toplevel;
	volatile uint32_t marked;
	volatile uint32_t fullylinked;
#if !defined(LL_GLOBAL_LOCK) && PTLOCK_SIZE < 64 /*fixes some crash apparently */
	volatile uint32_t padding;
#endif
#if !defined(LL_GLOBAL_LOCK)
	ptlock_t lock;
#endif
	volatile struct sl_marked* next[1];
};

template <typename K, typename V>
volatile sl_marked<K,V>* allocate_sl_marked_unlinked(K key, V val,
		unsigned int size_pad_32, int toplevel, int transactional)
{
	volatile sl_marked<K,V>* node;
#if GC == 1
	if (unlikely(transactional)) {
		// use levelmax instead of toplevel in order to be able to use the ssalloc allocator
		size_t node_size = size_pad_32;
		size_t node_size_rm = node_size & 63;
		if (node_size_rm) {
			node_size += 64 - node_size_rm;
		}
		node = (volatile sl_marked<K,V>*) malloc(node_size);
	} else {
		size_t node_size = size_pad_32;
#	if defined(DO_PAD)
		size_t node_size_rm = node_size & 63;
		if (node_size_rm) {
			node_size += 64 - node_size_rm;
		}
#	endif
		node = (volatile sl_marked<K,V>*) ssmem_alloc(alloc, node_size);
	}
#else
	size_t node_size = size_pad_32;
	if (transactional) {
		size_t node_size_rm  = node_size & 63;
		if (node_size_rm) {
			node_size += 64 - node_size_rm;
		}
	}

	node = (volatile sl_marked<K,V>*)malloc(node_size);
#endif
	node->key = key;
	node->val = val;
	node->toplevel = toplevel;
	node->marked = 0;
	node->fullylinked = 0;
	INIT_LOCK(ND_GET_LOCK(node));
#if defined(__tile__)
	MEM_BARRIER;
#endif
	return node;
}

template <typename K, typename V>
volatile sl_marked<K,V>* allocate_sl_marked(K key, V val,
		volatile sl_marked<K,V>* next, int levelmax,
		unsigned int size_pad_32, int toplevel, int transactional)
{
	volatile sl_marked<K,V> *node;
	node = allocate_sl_marked_unlinked(key, val, size_pad_32,
			toplevel, transactional);
	for (int i=0; i < toplevel; i++) {
		node->next[i] = next;
	}
	MEM_BARRIER;

	return node;
}

template <typename K, typename V>
void delete_sl_marked(volatile sl_marked<K,V> *node)
{
	DESTROY_LOCK(ND_GET_LOCK(node));
	// TODO seems weird
	ssfree_alloc(1,(void*)node);
}
#endif
