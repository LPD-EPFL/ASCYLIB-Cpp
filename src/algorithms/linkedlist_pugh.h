#ifndef _LINKED_LIST_PUGH_H_
#define _LINKED_LIST_PUGH_H_

#include <stdlib.h>
#include "utils.h"
#include "search.h"
#include "ll_locked.h"
#include "key_max_min.h"

#define PUGH_RO_FAIL RO_FAIL

template <typename K, typename V,
	 typename K_MAX_MIN = KeyMaxMin<K> >
class LinkedListPugh : public Search<K,V>
{
public:
	LinkedListPugh()
	{
		volatile ll_locked<K,V> *min, *max;
		max = initialize_ll_locked<K,V>(K_MAX_MIN::max_value(),
				(V) 0, (volatile ll_locked<K,V> *)NULL);
		min = initialize_ll_locked<K,V>(K_MAX_MIN::min_value(),
				(V) 0, max);
		head = min;
#if defined(LL_GLOBAL_LOCK)
		lock = (volatile ptlock_t *) malloc( sizeof( ptlock_t ) );
		if (NULL == lock) {
			perror("malloc @ LinkedListPugh constructor");
			exit(1);
		}
		GL_INIT_LOCK(lock);
#endif
		MEM_BARRIER;
	}

	~LinkedListPugh()
	{
		volatile ll_locked<K,V> *curr, *next;
		curr = head;
		while (NULL != curr) {
			next = curr->next;
			ll_locked_delete<K,V>( curr );
			curr = next;
		}
	}

	V search(K key)
	{
		PARSE_TRY();
		volatile ll_locked<K,V> *right = search_weak_right(key);
		if (right->key == key)
		{
			return right->val;
		}
		return false;
	}

	int insert(K key, V val)
	{
		PARSE_TRY();
		UPDATE_TRY();
		int result = 1;
		volatile ll_locked<K,V> *right;
#if PUGH_RO_FAIL == 1
		volatile ll_locked<K,V> *left = search_strong_cond(key, &right, 1);
		if (NULL == left) {
			return 0;
		}
#else
		volatile ll_locked<K,V> *left = search_strong(key, &right);
#endif
		if (right->key == key) {
			result = 0;
		} else {
			volatile ll_locked<K,V> *n = allocate_ll_locked<K,V>(
					key, val, left->next);
			left->next = n;
		}
		UNLOCK(ND_GET_LOCK(left));
		GL_UNLOCK(lock);
		return result;
	}

	V remove(K key)
	{
		PARSE_TRY();
		UPDATE_TRY();
		V result = 0;
		volatile ll_locked<K,V> *right;
#if PUGH_RO_FAIL == 1
		volatile ll_locked<K,V> *left = search_strong_cond(key, &right, 0);
		if (NULL == left) {
			return 0;
		}
#else
		volatile ll_locked<K,V> *left = search_strong(key, &right);
#endif
		if (right->key == key) {
			LOCK(ND_GET_LOCK(right));
			result = right->val;
			left->next = right->next;
			right->next = left;
			UNLOCK(ND_GET_LOCK(right));
			ll_locked_release(right);
		}
		UNLOCK(ND_GET_LOCK(left));
		GL_UNLOCK(lock);
		return result;
	}

	int length()
	{
		int count = 0;
		volatile ll_locked<K,V> *tmp;

		tmp = head->next;
		while(tmp->next != NULL) {
			count++;
			tmp = tmp->next;
		}
		return count;
	}

private:
	volatile ll_locked<K,V> *head;
	volatile ptlock_t *lock;

	inline volatile ll_locked<K,V> *search_weak_left(K key)
	{
		volatile ll_locked<K,V> *pred = head;
		volatile ll_locked<K,V> *succ = pred->next;
		while(succ->key < key) {
			pred = succ;
			succ = succ->next;
		}
		return pred;
	}
	inline volatile ll_locked<K,V> *search_weak_right(K key)
	{
		volatile ll_locked<K,V> *succ = head->next;
		while (succ->key < key) {
			succ = succ->next;
		}
		return succ;
	}

	inline volatile ll_locked<K,V> *search_strong(
			K key, volatile ll_locked<K,V> **right )
	{
		volatile ll_locked<K,V> *pred = search_weak_left(key);
		GL_LOCK(lock);
		LOCK(ND_GET_LOCK(pred));
		volatile ll_locked<K,V> *succ = pred->next;
		while(unlikely(succ->key < key)) {
			UNLOCK(ND_GET_LOCK(pred));
			pred = succ;
			LOCK(ND_GET_LOCK(pred));
			succ = pred->next;
		}
		*right = succ;
		return pred;
	}

	inline volatile ll_locked<K,V> *search_strong_cond(
			K key, volatile ll_locked<K,V> **right, int equal )
	{
		volatile ll_locked<K,V> *pred = search_weak_left(key);
		volatile ll_locked<K,V> *succ = pred->next;
		if ((succ->key == key) == equal) {
			return 0;
		}
		GL_LOCK(lock);
		LOCK(ND_GET_LOCK(pred));
		succ = pred->next;
		while(unlikely(succ->key < key)) {
			UNLOCK(ND_GET_LOCK(pred));
			pred = succ;
			LOCK(ND_GET_LOCK(pred));
			succ = pred->next;
		}
		*right = succ;
		return pred;
	}
};
#endif
