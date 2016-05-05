#ifndef _HASHTABLE_JAVA_H_
#define _HASHTABLE_JAVA_H_

#define DEFAULT_LOAD 1
#define MAXHTLENGTH 65536

#define CHM_READ_ONLY_FAIL RO_FAIL
#define CHM_NUM_SEGMENTS 128
#if defined(__tile__)
#	define CHM_LOAD_FACTOR 3
#else
#	define CHM_LOAD_FACTOR 1
#endif
#define CHM_TRY_PREFETCH 0
#define CHM_MAX_SCAN_RETRIES 64

#include<stdlib.h>
extern "C" {
#include"lock_if.h"
#include"utils.h"
}
#include"search.h"
#include"key_hasher.h"
#include"conc_hashmap_segment.h"
#include"ll_simple.h"

template<typename K, typename V>
class HashtableJavaCHM : public Search<K,V>
{
public:
	HashtableJavaCHM(size_t capacity, size_t num_segments)
	{
		chm = (conc_hashmap*)malloc(sizeof(conc_hashmap));
		assert(chm != NULL);

		if (capacity < num_segments) {
			capacity = num_segments;
		}
		chm->num_segments = num_segments;
		chm->segments = (volatile conc_hashmap_segment<K,V>**)
			malloc(chm->num_segments * sizeof(conc_hashmap*));
		assert(chm->segments != NULL);

		chm->hash = chm->num_segments - 1;
		int hs = floor_log_2(chm->num_segments);
		chm->hash_seed = hs;
		assert(capacity % chm->num_segments == 0);
		size_t capacity_seg = capacity / chm->num_segments;
#if defined(DEBUG)
		printf("#seg: %5zu, #bu/seg: %zu\n", chm->num_segments, capacity_seg);
#endif
		for (int s=0;s < chm->num_segments; s++) {
			chm->segments[s] = allocate_conc_hashmap_segment<K,V>(
					capacity_seg, (float)CHM_LOAD_FACTOR);
		}
	}

	~HashtableJavaCHM()
	{
	}

	int insert(K key, V val)
	{
		PARSE_TRY();
		UPDATE_TRY();

		volatile conc_hashmap_segment<K,V>* seg;
		volatile ptlock_t* seg_lock;
		int seg_num = key & chm->hash;
#if CHM_READ_ONLY_FAIL == 1
		seg = chm->segments[seg_num];
		if (chm_contains(seg, key) != 0) {
			return 0;
		}
#endif
#if CHM_TRY_PREFETCH == 1
		int walks = 0;
#endif
		LOCK_TRY_ONCE_CLEAR();
		do {
			seg = chm->segments[seg_num];
			seg_lock = &seg->lock;
#if CHM_TRY_PREFETCH == 1
			if (walks>0 && walks<=CHM_MAX_SCAN_RETRIES) {
				chm_put_prefetch(seg, chm->hash_seed, key);
			}
			walks++;
#endif
		} while (!TRYLOCK_A(seg_lock));

		volatile ll_simple<K,V>** bucket = &seg->table[
			hash(key, chm->hash_seed) & seg->hash];
		volatile ll_simple<K,V>* curr = *bucket;
		volatile ll_simple<K,V>* pred = NULL;
		while (curr != NULL) {
			if (curr->key == key) {
				UNLOCK_A(seg_lock);
				return 0;
			}
			pred = curr;
			curr = curr->next;
		}

		volatile ll_simple<K,V>* n = allocate_ll_simple(key, val,
				(volatile ll_simple<K,V>*)NULL);

		uint32_t sizepp = seg->size + 1;
		if (unlikely(sizepp >= seg->size_limit)) {
#if defined(DEBUG)
			printf("-[%3d]- seg size limit %u :: resize\n", seg_num, seg->size_limit);
#endif
			chm_segment_rehash(seg_num, n);
		} else {
			if (pred != NULL) {
				pred->next = n;
			} else {
				*bucket = n;
			}
			seg->size = sizepp;
			UNLOCK_A(seg_lock);
		}
		return 1;
	}

	V remove(K key)
	{
		PARSE_TRY();
		UPDATE_TRY();

		volatile conc_hashmap_segment<K,V>* seg;
		volatile ptlock_t* seg_lock;
		int seg_num = key & chm->hash;
#if CHM_READ_ONLY_FAIL == 1
		seg = chm->segments[seg_num];
		if (chm_contains(seg,key) == 0) {
			return 0;
		}
#else
#endif
#if CHM_TRY_PREFETCH == 1
		int walks = 0;
#endif
		LOCK_TRY_ONCE_CLEAR();
		do {
			seg = chm->segments[seg_num];
			seg_lock = &seg->lock;
#if CHM_TRY_PREFETCH == 1
			if (walks>0 && walks <= CHM_MAX_SCAN_RETRIES) {
				chm_rem_refetch(seg, chm->hash_seed, key);
			}
			walks++;
#endif
		} while(!TRYLOCK_A(seg_lock));

		volatile ll_simple<K,V>** bucket = &seg->table[hash(key,chm->hash_seed) & seg->hash];
		volatile ll_simple<K,V>* curr = *bucket;
		volatile ll_simple<K,V>* pred = NULL;
		while (curr != NULL) {
			if (curr->key == key) {
				if (pred != NULL) {
					pred->next = curr->next;
				} else {
					*bucket = curr->next;
				}
				ll_simple_release(curr);
				seg->size--;
				UNLOCK_A(seg_lock);
				return curr->val;
			}
			pred = curr;
			curr = curr->next;
		}
		UNLOCK_A(seg_lock);
		return 0;
	}

	V search(K key)
	{
		PARSE_TRY();

		volatile conc_hashmap_segment<K,V>* seg =
			chm->segments[key & chm->hash];
		volatile ll_simple<K,V>** bucket = &seg->table[
			hash(key,chm->hash_seed) & seg->hash];
		volatile ll_simple<K,V>* curr = *bucket;

		while(curr!=NULL) {
			if (curr->key == key) {
				return curr->val;
			}
			curr = curr->next;
		}
		return 0;
	}

	int length()
	{
		size_t count = 0;
		for (int s=0; s<chm->num_segments; s++) {
			volatile conc_hashmap_segment<K,V>* seg = chm->segments[s];
			for (int i=0; i<seg->num_buckets; i++) {
				volatile ll_simple<K,V>* curr = seg->table[i];
				while (curr != NULL) {
					count++;
					curr = curr->next;
				}
			}
		}
		return count;
	}
private:
	struct conc_hashmap {
		union {
			struct {
				size_t num_segments;
				size_t hash;
				int hash_seed;
				volatile conc_hashmap_segment<K,V> **segments;
			};
			uint8_t padding[CACHE_LINE_SIZE];
		};
	};

	volatile conc_hashmap *chm;

	static int floor_log_2(unsigned int n)
	{
		int pos = 0;
		if (n >= 1<<16) { n >>= 16; pos += 16; }
		if (n >= 1<< 8) { n >>=  8; pos +=  8; }
		if (n >= 1<< 4) { n >>=  4; pos +=  4; }
		if (n >= 1<< 2) { n >>=  2; pos +=  2; }
		if (n >= 1<< 1) {           pos +=  1; }
		return ((n == 0) ? (-1) : pos);
	}

	static inline int hash(K key, int hash_seed)
	{
		return key >> hash_seed;
	}

	void chm_segment_rehash(int seg_num, volatile ll_simple<K,V> *new_node)
	{
		volatile conc_hashmap_segment<K,V>** seg_old_pointer;
		volatile conc_hashmap_segment<K,V>* seg_old;
		volatile conc_hashmap_segment<K,V>* seg_new;

		seg_old_pointer = chm->segments+seg_num;
		seg_old = *seg_old_pointer;
		seg_new = allocate_conc_hashmap_segment<K,V>(
				seg_old->num_buckets<<1, seg_old->load_factor);
		int mask_new = seg_new->hash;

		for (int b=0; b<seg_old->num_buckets; b++) {
			volatile ll_simple<K,V>* curr = seg_old->table[b];
			if (curr != NULL) {
				volatile ll_simple<K,V>* next = curr->next;
				int idx = hash(curr->key, chm->hash_seed) & mask_new;
				if (NULL == next) {
					seg_new->table[idx] = curr;
				} else {
					volatile ll_simple<K,V>* last_run = curr;
					int last_idx = idx;
					volatile ll_simple<K,V>* last;
					for (last=next; last!=NULL; last=last->next) {
						int k = hash(
								last->key,
								chm->hash_seed)
							& mask_new;
						if (k != last_idx) {
							last_idx = k;
							last_run = last;
						}
					}
					seg_new->table[last_idx] = last_run;
					volatile ll_simple<K,V>* p;
					for (p=curr; p!=last_run; p=p->next) {
						int k = hash(p->key, chm->hash_seed)
							& mask_new;
						volatile ll_simple<K,V>* n =
							allocate_ll_simple(
								p->key, p->val,
								seg_new->table[k]);
						seg_new->table[k] = n;
						ll_simple_release(p);
					}
				}
			}
		}
		int new_idx = hash(new_node->key, chm->hash_seed) & mask_new;
		new_node->next = seg_new->table[new_idx];
		seg_new->table[new_idx] = new_node;

		seg_new->size = seg_old->size+1;
		chm->segments[seg_num] = seg_new;
#if GC == 1
		ssmem_release(alloc, (void*) seg_old);
		ssmem_release(alloc, seg_old->table);
#endif
	}

	inline int chm_contains(volatile conc_hashmap_segment<K,V>* seg, K key)
	{
		PARSE_TRY();

		volatile ll_simple<K,V>** bucket = &seg->table[
			hash(key, chm->hash_seed) & seg->hash];
		volatile ll_simple<K,V>* curr = *bucket;

		while (curr!=NULL) {
			if (curr->key == key) {
				return 1;
			}
			curr = curr->next;
		}
		return 0;
	}

	void UNUSED chm_put_prefetch(volatile conc_hashmap_segment<K,V>* seg,
			int hash_seed, K key)
	{
		volatile ll_simple<K,V>** bucket = &seg->table[hash(key,hash_seed) & seg->hash];
		volatile ll_simple<K,V>* curr = *bucket;
		volatile ll_simple<K,V>* pred = NULL;

		while(curr != NULL) {
			if (curr->key == key) {
				return;
			}
			pred = curr;
			curr = curr->next;
		}
		if (pred != NULL) {
			PREFETCHW(pred);
		} else {
			PREFETCHW(*bucket);
		}
	}

	void UNUSED chm_rem_prefetch(volatile conc_hashmap_segment<K,V>* seg,
			int hash_seed, K key)
	{
		volatile ll_simple<K,V>** bucket = &seg->table[hash(key,hash_seed) & seg->hash];
		volatile ll_simple<K,V>* curr = *bucket;
		volatile ll_simple<K,V>* pred = NULL;

		while(curr != NULL) {
			if (curr->key == key) {
				if (pred != NULL) {
					PREFETCHW(pred);
				} else {
					PREFETCHW(*bucket);
				}
				break;
			}
			pred = curr;
			curr = curr->next;
		}
	}
};

#endif
