#ifndef _CONC_HASHMAP_SEGMENT_H_
#define _CONC_HASHMAP_SEGMENT_H_

#include"ll_simple.h"
#include <stdlib.h>
#include "lock_if.h"

template<typename K, typename V>
struct conc_hashmap_segment {
	union {
		struct {
			size_t num_buckets;
			size_t hash;
			volatile ptlock_t lock;
			uint32_t modifications;
			uint32_t size;
			float load_factor;
			uint32_t size_limit;
			volatile ll_simple<K,V> **table;
		};
		uint8_t padding[CACHE_LINE_SIZE];
	};
};

template<typename K, typename V>
conc_hashmap_segment<K,V>* allocate_conc_hashmap_segment(
		size_t capacity, float load_factor)
{
	conc_hashmap_segment<K,V>* seg = (conc_hashmap_segment<K,V>*)
		malloc(sizeof(conc_hashmap_segment<K,V>));
	assert(seg != NULL);

	seg->table = (volatile ll_simple<K,V>**)malloc(capacity * sizeof(conc_hashmap_segment<K,V>*));
	assert(seg->table != NULL);

	seg->num_buckets = capacity;
	seg->hash = capacity - 1;
	seg->modifications = 0;
	seg->size = 0;
	seg->load_factor = load_factor;
	seg->size_limit = seg->load_factor * capacity;
	if (seg->size_limit == 0) {
		seg->size_limit = 1;
	}
	for (int i = 0; i < seg->num_buckets; i++) {
		seg->table[i] = NULL;
	}

	INIT_LOCK_A(&seg->lock);
	return seg;
}

template<typename K, typename V>
void delete_conc_hashmap_segment(conc_hashmap_segment<K,V>* seg)
{
	DESTROY_LOCK_A(&seg->lock);
	free(seg);

}

#endif
