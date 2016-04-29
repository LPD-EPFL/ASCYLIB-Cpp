#ifndef _LINKED_LIST_LAZY_H_
#define _LINKED_LIST_LAZY_H_

#include "search.h"
#include "ll_marked.h"
#include "key_max_min.h"
extern "C" {
#include "lock_if.h"
#include "ssmem.h"
}

#define LAZY_RO_LATEX RO_FAIL
#define DEFAULT_LOCKTYPE                2
#define DEFAULT_ALTERNATE               0
#define DEFAULT_EFFECTIVE               1

template <typename K, typename V,
	typename K_MAX_MIN = KeyMaxMin<K> >
class LinkedListLazy : public Search<K,V>
{
	private:
	volatile ll_marked<K,V> *head;
#if defined(LL_GLOBAL_LOCK)
	volatile ptlock_t *lock;
#endif

	inline int parse_validate(volatile ll_marked<K,V> *pred,
			volatile ll_marked<K,V> *curr)
	{
		return (!pred->marked && !curr->marked && pred->next == curr);
	}

	public:
	LinkedListLazy()
	{
		volatile ll_marked<K,V> *min, *max;
		max =  initialize_ll_marked(K_MAX_MIN::max_value(),
				(V) 0, (volatile ll_marked<K,V> *)NULL);
		min = initialize_ll_marked(K_MAX_MIN::min_value(),
				(V) 0, max);
		head = min;
#if defined(LL_GLOBAL_LOCK)
		lock = (volatile ptlock_t *) malloc( sizeof( ptlock_t ) );
		if (NULL == lock) {
			perror("malloc @ LinkedListLazy constructor");
			exit(1);
		}
		GL_INIT_LOCK(lock);
#endif
		MEM_BARRIER;
	}
	~LinkedListLazy()
	{
		volatile ll_marked<K,V> *curr, *next;
		curr = head;
		while (NULL != curr) {
			next = curr->next;
			ll_marked_delete<K,V>( curr );
			curr = next;
		}
	}

	V search(K key)
	{
		PARSE_TRY();
		volatile ll_marked<K,V>* curr = head;
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
		volatile ll_marked<K,V> *curr, *pred, *newnode;
		int result = -1;
		do {
			PARSE_TRY();
			pred = head;
			curr = pred->next;
			while(likely(curr->key < key)) {
				pred = curr;
				curr = curr->next;
			}
			UPDATE_TRY();
#if LAZY_RO_FAIL == 1
			if (curr->key == key) {
				if (unlikely(curr->marked)) {
					continue;
				}
				return false;
			}
#endif
			// only one of the two following lock lines will be defined
			GL_LOCK(lock);
			LOCK(ND_GET_LOCK(pred));

			if (parse_validate(pred, curr)) {
				result = (curr->key != key);
				if (result) {
					newnode = allocate_ll_marked(
						key, value, curr);
#ifdef __tile__
					MEM_BARRIER;
#endif
					pred->next = newnode;
				}
			}
			GL_UNLOCK(lock);
			UNLOCK(ND_GET_LOCK(pred));
		} while (result < 0);
		return result;
	}

	V remove(K key)
	{
		volatile ll_marked<K,V> *pred, *curr;
		V result = 0;
		int done = 0;

		do {
			PARSE_TRY();
			pred = head;
			curr = pred->next;
			while (likely(curr->key < key)) {
				pred = curr;
				curr = curr->next;
			}
			UPDATE_TRY();
#if LAZY_RO_RAIL == 1
			if (curr->key != key) {
				return false;
			}
#endif
			GL_LOCK(lock);
			LOCK(ND_GET_LOCK(pred));
			LOCK(ND_GET_LOCK(curr));

			if (parse_validate(pred, curr)) {
				if (key == curr->key) {
					result = curr->val;
					volatile ll_marked<K,V> *c_next = curr->next;
					curr->marked = 1;
					pred->next = c_next;

					ll_marked_release(curr);
				}
				done = 1;
			}

			GL_UNLOCK(lock);
			UNLOCK(ND_GET_LOCK(curr));
			UNLOCK(ND_GET_LOCK(pred));
		} while(!done);

		return result;
	}

	int length()
	{
		int count = 0;
		volatile ll_marked<K,V> *tmp;

		tmp = head->next;
		while(tmp->next != NULL) {
			count++;
			tmp = tmp->next;
		}

		return count;
	}
};

#endif
