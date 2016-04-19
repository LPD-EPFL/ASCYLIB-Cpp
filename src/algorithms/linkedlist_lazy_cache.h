#ifndef _LINKED_LIST_LAZY_CACHE_H_
#define _LINKED_LIST_LAZY_CACHE_H_

#include "search.h"
#include "linklist_node_marked.h"
#include "key_max_min.h"
extern "C" {
#include "latency.h"
#include "lock_if.h"
#include "ssmem.h"
}

#define LAZY_RO_LATEX RO_FAIL
#define DEFAULT_LOCKTYPE                2
#define DEFAULT_ALTERNATE               0
#define DEFAULT_EFFECTIVE               1


template <typename K, typename V,
	typename K_MAX_MIN = KeyMaxMin<K> >
class LinkedListLazyCache : public Search<K,V>
{
	private:
	volatile node_ll_marked<K,V> *head;
#if defined(LL_GLOBAL_LOCK)
	volatile ptlock_t *lock;
#endif
	__thread node_ll_marked_cache<K,V> node_last = {0,NULL};
	
	inline int parse_validate(volatile node_ll_marked<K,V> *pred,
			volatile node_ll_marked<K,V> *curr)
	{
		return (!pred->marked && !curr->marked && pred->next == curr);
	}

	inline int lazy_cache_validate(K key)
	{
		if (unlikely(!node_last.key)) {
			return lazy_find_pes(key);
		}
		int valid = key > node_last.key && !node_last.cached_node->marked;
		if (!valid) {
#if GC==1
			SSMEM_SAFE_TO_RECLAIM();
#endif
		} else {
			if (node_last.cached_node == NULL) {
				printf("valid==1, but node_last == 0\n");
			}
		}
		return valid;
	}

	inline int lazy_cache_validate_plus(K key)
	{
		if (unlikely(!node_last.key)) {
			return lazy_find_pes(key);
		}
		int valid = key >= node_last.key && !node_last.cached_node->marked;
		return valid;
	}

	inline void lazy_cache(volatile node_ll_marked<K,V> *pred)
	{
		if (pred->key == K_MAX_MIN::min_value()) {
			pred = pred->next; // next from the MIN value will be head
		}
		node_last.key = pred->key;
		node_last.cached_node = pred;
	}

	V lazy_find_pes(K key)
	{
		volatile node_ll_marked<K,V> *curr, *pred;
restart:
		PARSE_TRY();
		curr = head;
		do {
			pred = curr;
			curr = curr->next;
		} while (likely(curr->key < key));

		GL_LOCK(set->lock);
		LOCK(ND_GET_LOCK(pred));
		if (pred->marked) {
			GL_UNLOCK(lock);
			UNLOCK(ND_GET_LOCK(pred));
			goto restart;
		}

		int res = (curr->key == key);
		lazy_cache(pred);
		GL_UNLOCK(lock);
		UNLOCK(ND_GET_LOCK(pred));

		return res;
	}

	public:
	LinkedListLazyCache()
	{
		volatile node_ll_marked<K,V> *min, *max;
		max =  initialize_new_marked_ll_node(K_MAX_MIN::max_value(),
				(V) 0, (volatile node_ll_marked<K,V> *)NULL);
		min = initialize_new_marked_ll_node(K_MAX_MIN::min_value(),
				(V) 0, max);
		head = min;
#if defined(LL_GLOBAL_LOCK)
		//lock = (volatile ptlock_t *) ssaloc_aligned(CACHE_LINE_SIZE, sizeof(ptlock_t));
		lock = (volatile ptlock_t *) malloc( sizeof( ptlock_t ) );
		if (NULL == lock) {
			perror("malloc @ LinkedListLazy constructor");
			exit(1);
		}
		GL_INIT_LOCK(lock);
#endif
		MEM_BARRIER;
	}
	~LinkedListLazyCache()
	{
		volatile node_ll_marked<K,V> *curr, *next;
		curr = head;
		while (NULL != curr) {
			next = curr->next;
			node_ll_marked_delete( curr );
			curr = next;
		}
	}

	V search(K key)
	{
		PARSE_TRY();
		volatile node_ll_marked<K,V>* curr = head;
		if (lazy_cache_validate_plus(key)) {
			NODE_CACHE_HIT();
			curr = node_last.cached_node;
		} else {
			curr = head;
		}
		while (curr->key < key) {
			curr = curr->next;
		}

		sval_t res = 0;
		if ((curr->key == key) && !curr->marked) {
			res = curr->val;
		}

		return res;
	}

	int insert(K key, V value)
	{
		volatile node_ll_marked<K,V> *curr, *pred, *newnode;
		int result = -1;
		do {
			PARSE_TRY();
			if (lazy_cache_validate(key)) {
				NODE_CACHE_HIT();
				pred = node_last.cached_node;
			} else {
				pred = head;
			}
			curr = pred->next;
			while(likely(curr->key < key)) {
				pred = curr;
				curr = curr->next;
			}
			UPDATE_TRY();
#if LAZY_RO_FAIL == 1
			if (curr->key == key) {
				if (curr->marked) {
					continue;
				}
				lazy_cache(pred);
				return false;
			}
#endif
			// only one of the two following lock lines will be defined
			GL_LOCK(lock);
			LOCK(ND_GET_LOCK(pred));

			if (parse_validate(pred, curr)) {
				result = (curr->key != key);
				if (result) {
					newnode = allocate_new_marked_ll_node(
						key, value, curr);
#ifdef __tile__
					MEM_BARRIER;
#endif
					pred->next = newnode;
				}
			}
			lazy_cache(pred);
			GL_UNLOCK(lock);
			UNLOCK(ND_GET_LOCK(pred));
		} while (result < 0);
		return result;
	}

	V remove(K key)
	{
		volatile node_ll_marked<K,V> *pred, *curr;
		V result = 0;
		int done = 0;

		do {
			PARSE_TRY();
			if (lazy_cache_validate(key)) {
				NODE_CACHE_HIT();
				pred = node_last.cached_node;
			} else {
				pred = head;
			}
			curr = pred->next;
			while (likely(curr->key < key)) {
				pred = curr;
				curr = curr->next;
			}
			UPDATE_TRY();
#if LAZY_RO_RAIL == 1
			if (curr->key != key) {
				lazy_cache(pred);
				return false;
			}
#endif
			GL_LOCK(lock);
			LOCK(ND_GET_LOCK(pred));
			LOCK(ND_GET_LOCK(curr));

			if (parse_validate(pred, curr)) {
				if (key == curr->key) {
					result = curr->val;
					volatile node_ll_marked<K,V> *c_next = curr->next;
					curr->marked = 1;
					pred->next = c_next;

					node_ll_marked_release(curr);
				}
				done = 1;
			}
			lazy_cache(pred);
			
			GL_UNLOCK(lock);
			UNLOCK(ND_GET_LOCK(curr));
			UNLOCK(ND_GET_LOCK(pred));
		} while(!done);

		return result;
	}
	
	int length()
	{
		int count = 0;
		volatile node_ll_marked<K,V> *tmp;

		tmp = head->next;
		while(tmp->next != NULL) {
			count++;
			tmp = tmp->next;
		}

		return count;
	}
};

#endif
