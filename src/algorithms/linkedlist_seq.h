#ifndef _LINKED_LIST_SEQ_H_
#define _LINKED_LIST_SEQ_H_

#include <stdlib.h>
extern "C" {
#include "utils.h"
}
#include "key_max_min.h"
#include "search.h"
#include "ll_simple.h"

template <typename K, typename V,
	 typename K_MAX_MIN = KeyMaxMin<K> >
class LinkedListSeq : public Search<K,V>
{
public:
	LinkedListSeq()
	{
		volatile ll_simple<K,V> *min, *max;

		max = initialize_ll_simple<K,V>(
				K_MAX_MIN::max_value(), 0, NULL);
		min = initialize_ll_simple<K,V>(
				K_MAX_MIN::min_value(), 0, max);
		head = min;
		MEM_BARRIER;
	}

	~LinkedListSeq()
	{
		volatile ll_simple<K,V> *node, next;
		node = head;
		while (node != NULL) {
			next = node->next;
			free( (void*) node);
			node = next;
		}
	}

	V search(K key)
	{
		volatile ll_simple<K,V> *curr, *next;
		V res = 0;

		curr = head;
		next = curr->next;

		while(likely(next->key < key)) {
			curr = next;
			next = curr->next;
		}
		if (key == next->key) {
			res = next->val;
		}
		return res;
	}

	int insert(K key, V val)
	{
		volatile ll_simple<K,V> *curr, *next, *newnode;
		int found;
		curr = head;
		next = curr->next;

		while (likely(next->key < key)) {
			curr = next;
			next = curr->next;
		}
		found = (key == next->key);
		if (!found) {
			newnode = allocate_ll_simple<K,V>(key, val, next);
			curr->next = newnode;
		}
		return !found;
	}

	V remove(K key)
	{
		volatile ll_simple<K,V> *curr, *next;

		V res = 0;
		curr = head;
		next = curr->next;

		while(likely(next->key < key)) {
			curr = next;
			next = next->next;
		}

		if (key == next->key) {
			res = next->val;
			curr->next = next->next;
			ll_simple_release(next);
		}
		return res;
	}

	int length()
	{
		int count = 0;
		volatile ll_simple<K,V> *tmp;
		tmp = head->next;
		while (tmp->next != NULL) {
			count++;
			tmp = tmp->next;
		}
		return count;
	}

private:
	volatile ll_simple<K,V> *head;

};
#endif
