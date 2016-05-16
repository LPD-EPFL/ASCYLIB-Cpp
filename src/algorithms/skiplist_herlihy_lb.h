#ifndef _SKIPLIST_HERLIHY_LB_H_
#define _SKIPLIST_HERLIHY_LB_H_

#include"search.h"
#include"key_max_min.h"
#include"utils.h"
#include"latency.h"
#include"sl_marked.h"

#if LATENCY_PARSING == 1
__thread size_t lat_parsing_get = 0;
__thread size_t lat_parsing_put = 0;
__thread size_t lat_parsing_rem = 0;
#endif/* LATENCY_PARSING == 1 */

#define MAX_BACKOFF 131071
#define HERLIHY_MAX_MAX_LEVEL 64

template <typename K, typename V,
	 typename KEY_MAX_MIN = KeyMaxMin<K> >
class SkiplistHerlihyLB : public Search<K,V>
{
public:
	SkiplistHerlihyLB(unsigned int level_max)
	{
		volatile sl_marked<K,V> *min, *max;
		levelmax = level_max;
		size_pad_32 = sizeof(sl_marked<K,V>)
			+ levelmax * sizeof(sl_marked<K,V>*);
		while(size_pad_32 & 31) {
			size_pad_32++;
		}

		max = allocate_sl_marked(KEY_MAX_MIN::max_value(), (V) 0,
				(volatile sl_marked<K,V>*)NULL, levelmax,
				size_pad_32, levelmax, 1);
		min = allocate_sl_marked(KEY_MAX_MIN::min_value(), (V) 0,
				max, levelmax, size_pad_32, levelmax, 1);
		max->fullylinked = 1;
		min->fullylinked = 1;
		head = min;
#if defined(LL_GLOBAL_LOCK)
		lock = (volatile ptlock_t*) mallo(sizeof(ptlock_t));
		if (NULL == lock) {
			perror("malloc @ SkiplistHerlihyLB()");
			exit(1);
		}
		GL_INIT_LOCK(lock);
#endif
	}

	~SkiplistHerlihyLB()
	{
		volatile sl_marked<K,V> *node, *next;
		node = head;
		while (node != NULL) {
			next = node->next[0];
			delete_sl_marked(node);
			node = next;
		}
#if defined(LL_GLOBAL_LOCK)
		free((void*)lock);
#endif
	}

/*
 * Function optimistic_find corresponds to the contains method of the original
 * paper. In contrast with the original version, it allocates and frees the
 * memory at right places to avoid the use of a stop-the-world garbage
 * collector.
 */
	V search(K key)
	{
		V result = 0;
		PARSE_START_TS(0);
		volatile sl_marked<K,V> *node = optimistic_left_search(key);
		PARSE_END_TS(0, lat_parsing_get++);

		if (node != NULL && !node->marked && node->fullylinked) {
			result = node->val;
		}
		return result;
	}
/*
 * Function optimistic_insert stands for the add method of the original paper.
 * Unlocking and freeing the memory are done at the right places.
 */
	int insert(K key, V val)
	{
		volatile sl_marked<K,V> *succs[HERLIHY_MAX_MAX_LEVEL],
			 *preds[HERLIHY_MAX_MAX_LEVEL];
		volatile sl_marked<K,V> *node_found, *prev_pred, *new_node;
		volatile sl_marked<K,V> *pred, *succ;
		int toplevel, highest_locked, valid, found;
		unsigned int backoff;

		toplevel = get_rand_level();
		backoff = 1;

		PARSE_START_TS(1);
		while(1) {
			UPDATE_TRY();
			found = optimistic_search(key, preds, succs, 1);
			PARSE_END_TS(1, lat_parsing_put);

			if (found != -1) {
				node_found = succs[found];
				if (!node_found->marked) {
					while(!node_found->fullylinked) {
						PAUSE;
					}
					PARSE_END_INC(lat_parsing_put);
					return 0;
				}
				continue;
			}
			GL_LOCK(lock);
			highest_locked = -1;
			prev_pred = NULL;
			valid = 1;
			for (int i=0; valid && (i<toplevel); i++) {
				pred = preds[i];
				succ = succs[i];
				if (pred != prev_pred) {
					LOCK(ND_GET_LOCK(pred));
					highest_locked = i;
					prev_pred = pred;
				}
				valid = !pred->marked && !succ->marked &&
					((volatile sl_marked<K,V>*)
					 	pred->next[i] ==
					(volatile sl_marked<K,V>*) succ);
			}
			if (!valid) { // unlock predecessors before leaving
				// unlock global lock in the GL case
				unlock_levels(preds, highest_locked);
				if (backoff > 5000) {
					nop_rep(backoff & MAX_BACKOFF);
				}
				backoff <<= 1;
				continue;
			}
			new_node = allocate_sl_marked_unlinked(key, val,
					size_pad_32, toplevel, 0);
			for (int i=0; i < toplevel; i++) {
				new_node->next[i] = succs[i];
			}
#if defined(__tile__)
			MEM_BARRIER;
#endif
			for (int i=0; i < toplevel; i++) {
				preds[i]->next[i] = new_node;
			}
			new_node->fullylinked = 1;

			unlock_levels(preds, highest_locked);
			PARSE_END_INC(lat_parsing_put);
			return 1;
		}
	}

	V remove(K key)
	{
		volatile sl_marked<K,V> *succs[HERLIHY_MAX_MAX_LEVEL],
			*preds[HERLIHY_MAX_MAX_LEVEL];
		volatile sl_marked<K,V> *node_to_remove, *prev_pred;
		volatile sl_marked<K,V> *pred, *succ;
		int is_marked, toplevel, highest_locked, valid, found;
		unsigned int backoff;

		node_to_remove = NULL;
		is_marked = 0;
		toplevel = -1;
		backoff = 1;

		PARSE_START_TS(2);
		while (1) {
			UPDATE_TRY();
			found = optimistic_search(key, preds, succs, 1);
			PARSE_END_TS(2, lat_parsing_rem);

			if (is_marked || (found != -1
					 && ok_to_delete(succs[found], found))) {
				GL_LOCK(lock);
				if (!is_marked) {
					node_to_remove = succs[found];
					LOCK(ND_GET_LOCK(node_to_remove));
					toplevel = node_to_remove->toplevel;

					if (node_to_remove->marked) {
						GL_UNLOCK(lock);
						UNLOCK(ND_GET_LOCK(node_to_remove));
						PARSE_END_INC(lat_parsing_rem);
						return 0;
					}
					node_to_remove->marked = 1;
					is_marked = 1;
				}

				// Physical deletion
				highest_locked = -1;
				prev_pred = NULL;
				valid = 1;
				for (int i=0; valid && (i < toplevel); i++) {
					pred = preds[i];
					succ = succs[i];
					if (pred != prev_pred) {
						LOCK(ND_GET_LOCK(pred));
						highest_locked = i;
						prev_pred = pred;
					}
					valid = (!pred->marked &&
						((volatile sl_marked<K,V>*)
						 	pred->next[i] ==
						(volatile sl_marked<K,V>*) succ));
				}
				if (!valid) {
					unlock_levels(preds, highest_locked);
					if (backoff > 5000) {
						nop_rep(backoff & MAX_BACKOFF);
					}
					backoff <<= 1;
					continue;
				}
				for (int i=toplevel-1; i >= 0; i--) {
					preds[i]->next[i] =
						node_to_remove->next[i];
				}
				V val = node_to_remove->val;
#if GC == 1
				ssmem_free(alloc, (void*) node_to_remove);
#endif
				UNLOCK(ND_GET_LOCK(node_to_remove));
				unlock_levels(preds, highest_locked);

				PARSE_END_INC(lat_parsing_rem);
				return val;
			} else {
				PARSE_END_INC(lat_parsing_rem);
				return 0;
			}
		}
	}

	int length()
	{
		int count = 0;
		volatile sl_marked<K,V> *node;

		node = head->next[0];
		while (node->next[0]!=NULL) {
			if (node->fullylinked && !node->marked) {
				count++;
			}
			node = node->next[0];
		}
		return count;
	}
private:
	volatile sl_marked<K,V> *head;
#if defined(LL_GLOBAL_LOCK)
	volatile ptlock_t* lock;
#endif
	unsigned int levelmax;
	unsigned int size_pad_32;

	inline int get_rand_level()
	{
		int level = 1;
		for (int i=0; i<levelmax-1; i++) {
			if ((rand_range(100)-1)<50) {
				level++;
			} else {
				break;
			}
		}
		return level;
	}

	inline int ok_to_delete(volatile sl_marked<K,V> *node, int found)
	{
		return node->fullylinked && ((node->toplevel-1)==found)
			&& !node->marked;
	}

/*
 * Function optimistic_search corresponds to the findNode method of the
 * original paper. A fast parameter has been added to speed-up the search
 * so that the function quits as soon as the searched element is found.
 */
	inline int optimistic_search(K key, volatile sl_marked<K,V> **preds,
			volatile sl_marked<K,V> **succs, int fast)
	{
restart:
		PARSE_TRY();
		int found;
		volatile sl_marked<K,V> *pred, *curr;

		found = -1;
		pred = head;
		for (int i=(pred->toplevel - 1); i>=0; i--) {
			curr = pred->next[i];
			while (key > curr->key) {
				pred = curr;
				curr = pred->next[i];
			}
			if (preds != NULL) {
				preds[i] = pred;
				if (unlikely(pred->marked)) {
					goto restart;
				}
			}
			succs[i] = curr;
			if (found == -1 && key == curr->key) {
				found = i;
			}
		}
		return found;
	}

	inline volatile sl_marked<K,V>* optimistic_left_search(K key)
	{
		PARSE_TRY();
		volatile sl_marked<K,V> *pred, *curr, *node=NULL;
		pred = head;
		for (int i=(pred->toplevel-1); i>=0; i--) {
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

	inline void unlock_levels(volatile sl_marked<K,V> **nodes,
			int highestlevel)
	{
#if defined(LL_GLOBAL_LOCK)
		GL_UNLOCK(lock);
#else
		volatile sl_marked<K,V> *old = NULL;
		for (int i=0; i<=highestlevel; i++) {
			if (old != nodes[i]) {
				UNLOCK(ND_GET_LOCK(nodes[i]));
			}
			old = nodes[i];
		}
#endif
	}
};
#endif
