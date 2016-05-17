#ifndef _SKIPLIST_SEQ_H_
#define _SKIPLIST_SEQ_H_

#include"search.h"
#include"key_max_min.h"
#include"sl_simple.h"
#include"latency.h"
#if LATENCY_PARSING == 1
__thread size_t lat_parsing_get = 0;
__thread size_t lat_parsing_put = 0;
__thread size_t lat_parsing_rem = 0;
#endif  /* LATENCY_PARSING == 1 */

#define MAXLEVEL 32

template <typename K, typename V,
	 typename KEY_MAX_MIN = KeyMaxMin<K> >
class SkiplistSeq : public Search<K,V>
{
public:
	SkiplistSeq(unsigned int level_max)
	{
		volatile sl_simple<K,V> *min, *max;
		// moved code from test_simple here
		levelmax = level_max;
		size_pad_32 = sizeof(sl_simple<K,V>)
			+ levelmax * sizeof(sl_simple<K,V>*);
		while (size_pad_32 & 31) {
			size_pad_32++;
		}
		max = allocate_sl_simple(KEY_MAX_MIN::max_value(), (V) 0,
				(volatile sl_simple<K,V>*) NULL, levelmax,
				size_pad_32, levelmax, 1);
		min = allocate_sl_simple(KEY_MAX_MIN::min_value(), (V) 0,
				max, levelmax, size_pad_32, levelmax, 1);
		head= min;
	}

	~SkiplistSeq()
	{
		volatile sl_simple<K,V> *node, *next;

		node = head;
		while (node != NULL) {
			next = node->next[0];
			sl_delete_node(node);
			node = next;
		}
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

	V search(K key)
	{
		PARSE_START_TS(0);
		V result = 0;

		volatile sl_simple<K,V> *node, *next;
		node = head;
		for (int i=node->toplevel-1; i>=0; i--) {
			next = node->next[i];
			while (next->key < key) {
				node = next;
				next = node->next[i];
			}
		}
		node = node->next[0];
		PARSE_END_TS(0, lat_parsing_get++);
		if (node->key == key) {
			result = node->val;
		}
		return result;
	}

	int insert(K key, V val)
	{
		PARSE_START_TS(1);
		int l, result;
		volatile sl_simple<K,V> *node, *next;
		volatile sl_simple<K,V> *preds[MAXLEVEL], *succs[MAXLEVEL];

		node = head;
		for (int i=node->toplevel-1; i>=0; i--) {
			next = node->next[i];
			while (next->key < key) {
				node = next;
				next = node->next[i];
			}
			preds[i] = node;
			succs[i] = node->next[i];
		}
		node = node->next[0];
		PARSE_END_TS(1, lat_parsing_put++);

		result = node->key != key;
		if (result == 1) {
			l = get_rand_level();
			node = allocate_sl_simple_unlinked(key, val,
					size_pad_32, l, 0);
			for (int i=0; i < l; i++) {
				node->next[i] = succs[i];
#ifdef __tile__
				MEM_BARRIER;
#endif
				preds[i]->next[i] = node;
			}
		}
		return result;
	}

	V remove(K key)
	{
		PARSE_START_TS(2);
		V result = 0;
		volatile sl_simple<K,V> *node, *next = NULL;
		volatile sl_simple<K,V> *preds[MAXLEVEL], *succs[MAXLEVEL];

		node = head;
		for (int i=node->toplevel-1; i>=0; i--) {
			next = node->next[i];
			while (next->key < key) {
				node = next;
				next = node->next[i];
			}
			preds[i] = node;
			succs[i] = node->next[i];
		}
		PARSE_END_TS(2, lat_parsing_rem++);
		if (next->key == key) {
			result = next->val;
			for (int i=0; i<head->toplevel; i++) {
				if (succs[i]->key == key) {
					preds[i]->next[i] = succs[i]->next[i];
				}
			}
			sl_simple_delete(next);
		}
		return result;
	}

private:
	volatile sl_simple<K,V> *head;
	unsigned int levelmax;
	unsigned int size_pad_32;

	inline int get_rand_level()
	{
		int level = 1;
		for (int i = 0; i < levelmax - 1; i++)
		{
			if (rand_range(101) < 50) {
				level++;
			} else {
				break;
			}
		}
		/* 1 <= level <= levelmax */

		return level;
	}
};
#endif
