#ifndef _SKIPLIST_OPTIK_H_
#define _SKIPLIST_OPTIK_H_

#include"search.h"
#include"sl_locked.h"
#include"utils.h"
#include<stdlib.h>
#include"ssmem.h"

#include"latency.h"
#if LATENCY_PARSING == 1
__thread size_t lat_parsing_get = 0;
__thread size_t lat_parsing_put = 0;
__thread size_t lat_parsing_rem = 0;
#endif

#define MAX_BACKOFF 131071
#define OPTIK_MAX_MAX_LEVEL 64

template <typename K, typename V,
	 typename KEY_MAX_MIN = KeyMaxMin<K> >
class SkiplistOptik: public Search<K,V>
{
public:
	SkiplistOptik(unsigned int level_max)
	{
		volatile sl_locked<K,V> *min, *max;
		levelmax = level_max;
		size_pad_32 = sizeof(sl_locked<K,V>)
			+ levelmax * sizeof(sl_simple<K,V>*);
		while(size_pad_32 & 31) {
			size_pad_32++;
		}
		max = allocate_sl_locked(KEY_MAX_MIN::max_value(), (V) 0,
				(volatile sl_locked<K,V>*)NULL, levelmax,
				size_pad_32, levelmax, 1);
		min = allocate_sl_locked(KEY_MAX_MIN::min_value(), (V) 0,
				max, levelmax, size_pad_32, levelmax, 1);
		head = min;
	}

	~SkiplistOptik()
	{
		volatile sl_locked<K,V> *node, *next;
		node = head;
		while (node != NULL) {
			next = node->next[0];
			delete_sl_node(node);
			node = next;
		}
#if defined(LL_GLOBAL_LOCK)
		free((void*)lock);
#endif
	}

	int insert(K key, V val)
	{
		volatile sl_locked<K,V>* preds[OPTIK_MAX_MAX_LEVEL];
		volatile optik_t predsv[OPTIK_MAX_MAX_LEVEL], unused;
		volatile sl_locked<K,V>* node_new = NULL;

		int toplevel = get_rand_level();
		int inserted_upto = 0;
restart:
		UPDATE_TRY();
		volatile sl_locked<K,V>* node_found = sl_optik_search(key,
				preds, predsv, &unused);
		if (node_found != NULL && !inserted_upto) {
			if (!optik_is_deleted(node_found->lock)) {
				if (unlikely(node_new != NULL)) {
#if GC == 1
					ssmem_free(alloc, (void*) node_new);
#else
					free(node_new);
#endif
				}
				return 0;
			} else {
				// there is a logically deleted node
				// wait for it to be physically removed
				goto restart;
			}
		}
		if (node_new == NULL) {
			node_new = allocate_sl_locked_unlinked(key, val,
					size_pad_32, toplevel,0);
		}
		volatile sl_locked<K,V>* pred_prev = NULL;
		for (int i=inserted_upto; i<toplevel; i++) {
			volatile sl_locked<K,V>* pred = preds[i];
			if (pred_prev != pred &&
					!optik_trylock_version(&pred->lock, predsv[i])) {
				unlock_levels_down(preds, inserted_upto, i-1);
				inserted_upto = i;
				goto restart;
			}
			node_new->next[i] = pred->next[i];
			pred->next[i] = node_new;
			pred_prev = pred;
		}
		node_new->state = 1;
		unlock_levels_down(preds, inserted_upto, toplevel-1);

		return 1;
	}

	V remove(K key)
	{
		volatile sl_locked<K,V>* preds[OPTIK_MAX_MAX_LEVEL];
		volatile optik_t predsv[OPTIK_MAX_MAX_LEVEL], node_foundv;
		int my_delete = 0;
restart:
		UPDATE_TRY();
		volatile sl_locked<K,V>* node_found =
			sl_optik_search(key, preds, predsv, &node_foundv);
		if (NULL == node_found) {
			return 0;
		}
		if (!my_delete) {
			if (optik_is_deleted(node_found->lock)
					|| (!node_found->state)) {
				return 0;
			}
			if (!optik_trylock_vdelete(&node_found->lock,
						node_foundv)) {
				if (optik_is_deleted(node_found->lock)) {
					return 0;
				} else {
					goto restart;
				}
			}
		}
		my_delete = 1;

		const int toplevel_nf = node_found->toplevel;
		volatile sl_locked<K,V>* pred_prev = NULL;
		for (int i=0; i < toplevel_nf; i++) {
			volatile sl_locked<K,V> *pred = preds[i];
			if (pred_prev != pred &&
					!optik_trylock_version(&pred->lock,
						predsv[i])) {
				unlock_levels_down(preds, 0, i-1);
				goto restart;
			}
			pred_prev = pred;
		}

		for (int i=0; i < toplevel_nf; i++) {
			preds[i]->next[i] = node_found->next[i];
		}
		unlock_levels_down(preds, 0, toplevel_nf-1);
#if GC == 1
		ssmem_free(alloc, (void*)node_found);
#endif
		return node_found->val;
	}

	int length()
	{
		int count = 0;
		volatile sl_locked<K,V> *tmp;
		tmp = head->next[0];
		while (tmp->next[0] != NULL) {
			if (!optik_is_deleted(tmp->lock)) {
				count++;
			}
			tmp = tmp->next[0];
		}
		return count;
	}

	V search(K key)
	{
		PARSE_START_TS(0);
		volatile sl_locked<K,V> *node = sl_optik_left_search(key);
		PARSE_END_TS(0, lat_parsing_get++);

		V result = 0;
		if (node != NULL && !optik_is_deleted(node->lock)) {
			result = node->val;
		}
		return result;
	}

private:
	volatile sl_locked<K,V> *head;
	unsigned int levelmax;
	unsigned int size_pad_32;

	volatile sl_locked<K,V>* sl_optik_search(K key,
			volatile sl_locked<K,V> **preds,
			volatile optik_t* predsv, volatile optik_t *node_foundv)
	{
restart:
		PARSE_TRY();

		volatile sl_locked<K,V>* node_found = NULL;
		volatile sl_locked<K,V>* pred = head;
		optik_t predv = head->lock;

		for (int i=pred->toplevel-1; i>=0; i--) {
			volatile sl_locked<K,V>* curr = pred->next[i];
			optik_t currv = curr->lock;

			while (key > curr->key) {
				predv = currv;
				pred = curr;

				curr = pred->next[i];
				currv = curr->lock;
			}

			if (unlikely(optik_is_deleted(predv))) {
				goto restart;
			}
			preds[i] = pred;
			predsv[i] = predv;
			if (key == curr->key) {
				node_found = curr;
				*node_foundv = currv;
			}
		}
		return node_found;
	}

	inline volatile sl_locked<K,V>* sl_optik_left_search(K key)
	{
		PARSE_TRY();
		volatile sl_locked<K,V> *pred, *curr, *node = NULL;
		pred = head;

		for (int i=pred->toplevel-1; i>=0; i--) {
			curr = pred->next[i];
			while (key > curr->key) {
				pred = curr;
				curr = pred->next[i];
			}
			if (key == curr->key) {
				node = curr;
				break;
			}
		}
		return node;
	}

	inline void unlock_levels_down(volatile sl_locked<K,V>** nodes,
			int low, int high)
	{
		volatile sl_locked<K,V>* old = NULL;
		for (int i=high; i >= low; i--) {
			if (old != nodes[i]) {
				optik_unlock(&nodes[i]->lock);
			}
			old = nodes[i];
		}
	}

	inline void unlock_levels_up(volatile sl_locked<K,V>** nodes,
			int low, int high)
	{
		volatile sl_locked<K,V>* old = NULL;
		for (int i=low; i < high; i++) {
			if (old != nodes[i]) {
				optik_unlock(&nodes[i]->lock);
			}
			old = nodes[i];
		}
	}

	inline int get_rand_level()
	{
		int level = 1;
		for (int i=0;i<levelmax-1;i++) {
			if (rand_range(100)-1 < 50) {
				level++;
			} else {
				break;
			}
		}
		return level;

	}

};

#endif
