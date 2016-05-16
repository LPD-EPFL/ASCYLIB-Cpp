#ifndef _SKIPLIST_HERLIHY_LF_H_
#define _SKIPLIST_HERLIHY_LF_H_

#include<atomic_ops.h>
#include"atomic_ops_if.h"
#include"search.h"
#include"key_max_min.h"
#include"utils.h"
#include"latency.h"
#include"sl_simple.h"

#if LATENCY_PARSING == 1
__thread size_t lat_parsing_get = 0;
__thread size_t lat_parsing_put = 0;
__thread size_t lat_parsing_rem = 0;
#endif/* LATENCY_PARSING == 1 */

#define MAX_BACKOFF 131071
#define HERLIHY_MAX_MAX_LEVEL 64

template <typename K, typename V,
	 typename KEY_MAX_MIN = KeyMaxMin<K> >
class SkiplistHerlihyLF : public Search<K,V>
{
public:
	SkiplistHerlihyLF(unsigned int level_max)
	{
		volatile sl_simple<K,V> *min, *max;
		levelmax = level_max;
		size_pad_32 = sizeof(sl_simple<K,V>)
			+ levelmax * sizeof(sl_simple<K,V>*);
		while(size_pad_32 & 31) {
			size_pad_32++;
		}

		max = allocate_sl_simple(KEY_MAX_MIN::max_value(), (V) 0,
				(volatile sl_simple<K,V>*)NULL, levelmax,
				size_pad_32, levelmax, 1);
		min = allocate_sl_simple(KEY_MAX_MIN::min_value(), (V) 0,
				max, levelmax, size_pad_32, levelmax, 1);
		head = min;
	}

	~SkiplistHerlihyLF()
	{
		volatile sl_simple<K,V> *node, *next;
		node = head;
		while (node != NULL) {
			next = node->next[0];
			sl_simple_delete(node);
			node = next;
		}
	}

	V search(K key)
	{
		V result = 0;
		PARSE_START_TS(0);
		volatile sl_simple<K,V> *left = herlihy_left_search(key);
		PARSE_END_TS(0, lat_parsing_get++);

		if (left->key == key) {
			result = left->val;
		}
		return result;
	}

	int insert(K key, V val)
	{
		volatile sl_simple<K,V> *new_node, *pred, *succ;
		volatile sl_simple<K,V> *succs[HERLIHY_MAX_MAX_LEVEL],
			 *preds[HERLIHY_MAX_MAX_LEVEL];
		int found;
		PARSE_START_TS(1);
retry:
		UPDATE_TRY();

		found = herlihy_search_no_cleanup(key, preds, succs);
		PARSE_END_TS(1, lat_parsing_put);

		if (found) {
			PARSE_END_INC(lat_parsing_put);
			return false;
		}
		new_node = allocate_sl_simple_unlinked(key, val, size_pad_32,
				get_rand_level(), 0);
		for (int i=0; i < new_node->toplevel; i++) {
			new_node->next[i] = succs[i];
		}
#if defined(__tile__)
		MEM_BARRIER;
#endif
		if (!ATOMIC_CAS_MB(&preds[0]->next[0],
					(volatile sl_simple<K,V>*)
						unset_mark((uintptr_t)succs[0]),
					new_node)) {
			sl_simple_delete(new_node);
			goto retry;
		}
		for (int i=1; i<new_node->toplevel; i++) {
			while (1) {
				pred = preds[i];
				succ = succs[i];
				if (is_marked((uintptr_t)new_node->next[i])) {
					PARSE_END_INC(lat_parsing_put);
					return true;
				}
				if (ATOMIC_CAS_MB(&pred->next[i], succ,
							new_node)) {
					break;
				}
				herlihy_search(key, preds, succs);
			}
		}
		PARSE_END_INC(lat_parsing_put);
		return true;
	}

	V remove(K key)
	{
		UPDATE_TRY();

		volatile sl_simple<K,V>* succs[HERLIHY_MAX_MAX_LEVEL];
		V result = 0;

		PARSE_START_TS(2);
		int found = herlihy_search_no_cleanup_succs(key, succs);
		PARSE_END_TS(2, lat_parsing_rem++);

		if (!found) {
			return false;
		}

		volatile sl_simple<K,V> *node_del = succs[0];
		int my_delete = mark_node_ptrs(node_del);
		if (my_delete) {
			result = node_del->val;
			herlihy_search(key, NULL, NULL);
#if GC == 1
			ssmem_free(alloc, (void*)succs[0]);
#endif
		}
		return result;
	}

	int length()
	{
		int count = 0;
		volatile sl_simple<K,V> *node;

		node = (volatile sl_simple<K,V>*)
			unset_mark((uintptr_t)head->next[0]);
		while (node->next[0]!=NULL) {
			if (!is_marked((uintptr_t)node->next[0])) {
				count++;
			}
			node = (volatile sl_simple<K,V>*)
				unset_mark((uintptr_t)node->next[0]);
		}
		return count;
	}
private:
	volatile sl_simple<K,V> *head;
#if defined(LL_GLOBAL_LOCK)
	volatile ptlock_t* lock;
#endif
	unsigned int levelmax;
	unsigned int size_pad_32;

	inline int get_rand_level()
	{
		int level = 1;
		for (int i=0; i<levelmax-1; i++) {
			if (rand_range(101)<50) {
				level++;
			} else {
				break;
			}
		}
		return level;
	}

	inline int herlihy_search(K key, volatile sl_simple<K,V> **left_list,
			volatile sl_simple<K,V> **right_list)
	{
		volatile sl_simple<K,V> *left, *left_next, *right = NULL,
			*right_next;
retry:
		PARSE_TRY();

		left = head;
		for (int i=levelmax-1; i>=0; i--) {
			left_next = left->next[i];
			if (is_marked((uintptr_t)left_next)) {
				goto retry;
			}
			// Find unmarked node pair at this level
			for (right = left_next; ; right = right_next) {
				// Skip a sequence of marked nodes
				right_next = right->next[i];
				while (is_marked((uintptr_t)right_next)) {
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
			//Ensure left and right nodes are adjacent
			if ((left_next != right) &&
					(!ATOMIC_CAS_MB(&left->next[i],
							left_next, right))) {
				goto retry;
			}
			if (left_list != NULL) {
				left_list[i] = left;
			}
			if (right_list != NULL) {
				right_list[i] = right;
			}
		}
		return right->key == key;
	}

	int herlihy_search_no_cleanup(K key, volatile sl_simple<K,V> **left_list,
			volatile sl_simple<K,V> **right_list) {
		PARSE_TRY();
		volatile sl_simple<K,V> *left, *left_next, *right=NULL;

		left = head;
		for (int i=levelmax-1; i>=0; i--) {
			left_next = (volatile sl_simple<K,V>*)
				unset_mark((uintptr_t)left->next[i]);
			right = left_next;
			while (1) {
				if (likely(!is_marked(
						(uintptr_t)right->next[i]))) {
					if (right->key >= key) {
						break;
					}
					left = right;
				}
				right = (volatile sl_simple<K,V>*)
					unset_mark((uintptr_t)right->next[i]);
			}
			left_list[i] = left;
			right_list[i] = right;
		}
		return right->key == key;
	}

	int herlihy_search_no_cleanup_succs(K key,
			volatile sl_simple<K,V> **right_list)
	{
		PARSE_TRY();

		volatile sl_simple<K,V> *left, *left_next, *right = NULL;
		left = head;
		for (int i=levelmax-1; i>=0; i--) {
			left_next = (volatile sl_simple<K,V>*)
				unset_mark((uintptr_t)left->next[i]);
			right = left_next;
			while (1) {
				if (likely(!is_marked(
						(uintptr_t)right->next[i]))) {
					if (right->key >= key) {
						break;
					}
					left = right;
				}
				right = (volatile sl_simple<K,V>*)
					unset_mark((uintptr_t)right->next[i]);
			}
			right_list[i] = right;
		}
		return right->key == key;
	}

	volatile sl_simple<K,V> *herlihy_left_search(K key)
	{
		PARSE_TRY();
		volatile sl_simple<K,V> *left = NULL;
		volatile sl_simple<K,V> *left_prev;

		left_prev = head;
		for (int level = levelmax-1; level>=0; level--) {
			left = (volatile sl_simple<K,V>*)
				unset_mark((uintptr_t)left_prev->next[level]);
			while (left->key < key ||
					is_marked((uintptr_t)
						left->next[level])) {
				if (!is_marked((uintptr_t)left->next[level])) {
					left_prev = left;
				}
				left = (volatile sl_simple<K,V>*)
					unset_mark((uintptr_t)left->next[level]);
			}
			if (left->key == key) {
				break;
			}
		}
		return left;
	}

	inline int mark_node_ptrs(volatile sl_simple<K,V> *node)
	{
		int cas = 0;
		volatile sl_simple<K,V> *node_next;

		for (int i=node->toplevel-1; i >= 0; i--) {
			do {
				node_next = node->next[i];
				if (is_marked((uintptr_t)node_next)) {
					cas = 0;
					break;
				}
				cas = ATOMIC_CAS_MB(&node->next[i],
						(volatile sl_simple<K,V>*)
						unset_mark((uintptr_t)node_next),
						set_mark((uintptr_t)node_next));
			} while (!cas);
		}
		return cas; // if I was the one that marked level 0
	}
};
#endif
