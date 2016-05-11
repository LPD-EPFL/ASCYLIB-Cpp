#ifndef _SKIPLIST_FRASER_H_
#define _SKIPLIST_FRASER_H_

#define MAXLEVEL 32
#define FRASER_MAX_MAX_LEVEL 64

#include "latency.h"
#include "key_max_min.h"
#include "search.h"
#include "sl_simple.h"

template <typename K, typename V,
	 typename KEY_MAX_MIN = KeyMaxMin<K> >
class SkiplistFraser : public Search<K,V>
{
public:
	SkiplistFraser(unsigned int level_max)
	{
		volatile sl_simple<K,V> *min, *max;
		// moved code from test_simple here
		levelmax = level_max;
		size_pad_32 = sizeof(sl_simple<K,V>)
			+ levelmax * sizeof(sl_simple<K,V>*);
		while (size_pad_32 & 31) {
			size_pad_32++;
		}
		// even though levelmax is used twice here
		// sometimes the toplevel parameter is NOT levelmax
		// which is why they are maintained as separate parameters
		max = sl_simple_allocate(KEY_MAX_MIN::max_value(), (V) 0,
				(volatile sl_simple<K,V>*)NULL, levelmax,
				size_pad_32, levelmax, 1);
		min = sl_simple_allocate(KEY_MAX_MIN::min_value(), (V) 0,
				max, levelmax, size_pad_32, levelmax, 1);
		head = min;
	}

	~SkiplistFraser()
	{
		sl_simple<K,V> *node, *next;

		node = head;
		while (node != NULL) {
			next = node->next[0];
			sl_simple_delete(node);
			node = next;
		}
	}

	V search(K key)
	{
		volatile sl_simple<K,V> *succs[FRASER_MAX_MAX_LEVEL];
		V result = 0;
		PARSE_START_TS(0);
		fraser_search(key, NULL, succs);
		PARSE_END_TS(0, lat_parsing_get++);

		if (succs[0]->key == key && !succs[0]->deleted) {
			result = succs[0]->val;
		}
		return result;
	}

	int insert(K key, V val)
	{
		volatile sl_simple<K,V> *new_node, *new_next, *pred, *succ;

		volatile sl_simple<K,V> *succs[FRASER_MAX_MAX_LEVEL],
			*preds[FRASER_MAX_MAX_LEVEL];
		int result = 0;
		new_node = sl_simple_initialize(key, val, size_pad_32,
				get_rand_level(), 0);
		PARSE_START_TS(1);
retry:
		UPDATE_TRY();
		fraser_search(key, preds, succs);
		PARSE_END_TS(1, lat_parsing_put);

		if (succs[0]->key == key) {
			if (succs[0]->deleted) {
				mark_node_ptrs(succs[0]);
				goto retry;
			}
			result = 0;
			sl_simple_delete(new_node);
			goto end;
		}
		for (int i=0; i < new_node->toplevel; i++) {
			new_node->next[i] = succs[i];
		}
#if defined(__tile__)
		MEM_BARRIER;
#endif
		if (!ATOMIC_CAS_MB(&preds[0]->next[0], succs[0], new_node)) {
			goto retry;
		}
		for (int i=1; i < new_node->toplevel; i++) {
			while(1) {
				pred = preds[i];
				succ = succs[i];
				new_next = new_node->next[i];
				if (is_marked((uintptr_t)new_next)) {
					goto success;
				}
				if ( (new_next != succ) &&
						(!ATOMIC_CAS_MB(&new_node->next[i],
						unset_mark((uintptr_t)new_next),
						succ))) {
					break;
				}
				if (succ->key == key) {
					succ = (sl_simple<K,V>*)
						unset_mark((uintptr_t) succ->next);
				}
				if (ATOMIC_CAS_MB(&pred->next[i], succ, new_node)) {
					break;
				}
				fraser_search(key, preds, succs);
			}
		}
success:
		result = 1;
end:
		PARSE_END_INC(lat_parsing_put);
		return result;
	}

	V remove(K key)
	{
		volatile sl_simple<K,V> *succs[FRASER_MAX_MAX_LEVEL];
		V result = 0;

		UPDATE_TRY();

		PARSE_START_TS(2);
		fraser_search(key, NULL, succs);
		PARSE_END_TS(2, lat_parsing_rem++);

		if (succs[0]->key != key) {
			goto end;
		}
		if (succs[0]->deleted) {
			goto end;
		}

		if (ATOMIC_FETCH_AND_INC_FULL(&succs[0]->deleted) == 0) {
			mark_node_ptrs(succs[0]);
			result = succs[0]->val;
#if GC == 1
			ssmem_free(alloc, (void*)succs[0]);
#endif
			fraser_search(key, NULL, NULL);
		}
end:
		return result;
	}

	int length()
	{
		int count = 0;
		volatile sl_simple<K,V> *node;
		node = (volatile sl_simple<K,V>*)
			unset_mark( (uintptr_t)head->next[0]);
		while (node->next[0] != NULL) {
			if (!is_marked((uintptr_t) node->next[0])) {
				count++;
			}
			node = (sl_simple<K,V>*)
				unset_mark((uintptr_t) node->next[0]);
		}

		return count;
	}
private:
	volatile sl_simple<K,V>* head;
	unsigned int levelmax;
	unsigned int size_pad_32;

	void fraser_search(K key, volatile sl_simple<K,V> **left_list,
			volatile sl_simple<K,V> **right_list)
	{
		volatile sl_simple<K,V> *left, *left_next, *right, *right_next;
retry:
		PARSE_TRY();
		left = head;
		for (int i=levelmax-1; i>=0; i--) {
			left_next = left->next[i];
			if (unlikely(is_marked((uintptr_t)left_next))) {
				goto retry;
			}
			for (right = left_next; ; right = right_next) {
				right_next = right->next[i];
				while(unlikely(is_marked((uintptr_t)right_next))) {
					right = (volatile sl_simple<K,V>*)
						unset_mark((uintptr_t)right_next);
					right_next = right->next[i];
				}
				if (right->key >= key) {
					break;
				}
				left = right;
				left_next = right_next;
			}
			if (left_next != right) {
				if (!ATOMIC_CAS_MB(&left->next[i], left_next, right)) {
					CLEANUP_TRY();
					goto retry;
				}
			}
			if (left_list != NULL) {
				left_list[i] = left;
			}
			if (right_list != NULL) {
				right_list[i] = right;
			}
		}
	}

	inline void mark_node_ptrs(volatile sl_simple<K,V> *node)
	{
		volatile sl_simple<K,V> *node_next;

		for (int i = node->toplevel-1; i>=0; i--) {
			do {
				node_next = node->next[i];
				if (is_marked((uintptr_t)node_next)) {
					break;
				}
			} while(!ATOMIC_CAS_MB(&node->next[i], node_next,
						set_mark((uintptr_t)node_next)));
		}
	}

	inline int get_rand_level()
	{
		int level=1;
		for (int i=0; i < levelmax-1; i++) {
			if (rand_range(101) < 50) {
				level++;
			} else {
				break;
			}
		}
		return level;
	}
};

#endif
