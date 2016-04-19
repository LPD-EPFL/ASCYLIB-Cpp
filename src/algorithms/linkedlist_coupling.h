#ifndef _LINKEDLIST_COUPLING_H_
#define _LINKEDLIST_COUPLING_H_

#include "key_max_min.h"
#include "search.h"
#include "linklist_node_marked.h"

template<typename K, typename V,
	typename K_MAX_MIN = KeyMaxMin<K> >
class LinkedListCoupling : public Search<K,V>
{
private:
	volatile node_ll_marked<K,V> *head;
#if defined(LL_GLOBAL_LOCK)
	volatile ptlock_t *lock;
#endif

public:
	LinkedListCoupling()
	{
		volatile node_ll_marked<K,V> *min, *max;
		max =  initialize_new_marked_ll_node(K_MAX_MIN::max_value(),
			(V) 0, (volatile node_ll_marked<K,V> *)NULL);
		min = initialize_new_marked_ll_node(K_MAX_MIN::min_value(),
			(V) 0, max);
		head = min;
#if defined(LL_GLOBAL_LOCK)
		lock = (volatile ptlock_t *) malloc( sizeof( ptlock_t ) );
		if (NULL == lock) {
			perror("malloc @ LinkedListCoupling constructor");
			exit(1);
		}
		GL_INIT_LOCK(lock);
#endif
		MEM_BARRIER;
	}

	~LinkedListCoupling()
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

		volatile node_ll_marked<K,V> *curr, *next; 
		V res = (V)0;

		GL_LOCK(lock); /* when GL_[UN]LOCK is defined the [UN]LOCK is not ;-) */
		LOCK(ND_GET_LOCK(head));
		curr = head;
		LOCK(ND_GET_LOCK(curr->next));
		next = curr->next;

		while (next->key < key) 
		{   
			UNLOCK(ND_GET_LOCK(curr));
			curr = next;
			LOCK(ND_GET_LOCK(next->next));
			next = curr->next;
		}   
		if (key == next->key)
		{   
			res = next->val;
		}   
		GL_UNLOCK(lock);
		UNLOCK(ND_GET_LOCK(curr));
		UNLOCK(ND_GET_LOCK(next));
		return res;
	}

	int insert(K key, V val)
	{
		PARSE_TRY();
		UPDATE_TRY();

		volatile node_ll_marked<K,V> *curr, *next, *newnode;
		int found;

		GL_LOCK(lock);           /* when GL_[UN]LOCK is defined the [UN]LOCK is not ;-) */
		LOCK(ND_GET_LOCK(head));
		curr = head;
		LOCK(ND_GET_LOCK(curr->next));
		next = curr->next;

		while (next->key < key)
		{
			UNLOCK(ND_GET_LOCK(curr));
			curr = next;
			LOCK(ND_GET_LOCK(next->next));
			next = curr->next;
		}
		found = (key == next->key);
		if (!found)
		{
			newnode = initialize_new_marked_ll_node<K,V>(
					key, val, next);
#ifdef __tile__
			MEM_BARRIER;
#endif
			curr->next = newnode;
		}
		GL_UNLOCK(lock);
		UNLOCK(ND_GET_LOCK(curr));
		UNLOCK(ND_GET_LOCK(next));
		return !found;
	}

	V remove(K key)
	{
		PARSE_TRY();
		UPDATE_TRY();

		volatile node_ll_marked<K,V> *curr, *next;
		V res = (V)0;

		GL_LOCK(lock); /* when GL_[UN]LOCK is defined the [UN]LOCK is not ;-) */
		LOCK(ND_GET_LOCK(head));
		curr = head;
		LOCK(ND_GET_LOCK(curr->next));
		next = curr->next;

		while (next->key < key) {   
			UNLOCK(ND_GET_LOCK(curr));
			curr = next;
			LOCK(ND_GET_LOCK(next->next));
			next = next->next;
		}   

		if (key == next->key) {   
			res = next->val;
			curr->next = next->next;
			UNLOCK(ND_GET_LOCK(next));
			node_ll_marked_delete<K,V>(next);
			UNLOCK(ND_GET_LOCK(curr));
		} else {   
			UNLOCK(ND_GET_LOCK(curr));
			UNLOCK(ND_GET_LOCK(next));
		}   
		GL_UNLOCK(lock);

		return res;

	}

	int length()
	{
		int size = 0;

		volatile node_ll_marked<K,V> *node;
		node = head->next;
		while(node->next != NULL) {
			size++;
			node = node->next;
		}
		return size;
	}
};

#endif
