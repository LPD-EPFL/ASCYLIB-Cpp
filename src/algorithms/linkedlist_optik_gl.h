#ifndef _LINKED_LIST_OPTIK_GL_H_
#define _LINKED_LIST_OPTIK_GL_H_

#include "optik.h"
#include "utils.h"
#include "search.h"
#include "key_max_min.h"
#include "ll_simple.h"

template <typename K, typename V,
	 typename K_MAX_MIN = KeyMaxMin<K> >
class LinkedListOptikGL : public Search<K,V>
{
public:
	LinkedListOptikGL()
	{
		volatile ll_simple<K,V> *min, *max;
		max =  initialize_ll_simple<K,V>(K_MAX_MIN::max_value(),
			(V) 0, (volatile ll_simple<K,V> *)NULL);
		min = initialize_ll_simple<K,V>(K_MAX_MIN::min_value(),
			(V) 0, max);
		head = min;

		optik_init(&lock);

		MEM_BARRIER;
	}

	~LinkedListOptikGL()
	{
		volatile ll_simple<K,V> *curr, *next;
		curr = head;
		while (NULL != curr) {
			next = curr->next;
			ll_simple_delete<K,V>( curr );
			curr = next;
		}
	}

	V search(K key)
	{
		PARSE_TRY();
		volatile ll_simple<K,V> *curr = head;

		while(likely(curr->key < key)) {
			curr = curr->next;
		}
		V res = (V)0;
		if (curr->key==key) {
			res = curr->val;
		}
		return res;
	}

	int insert(K key, V val)
	{
		volatile ll_simple<K,V> *curr, *pred, *newnode;
		NUM_RETRIES();

restart:
		PARSE_TRY();
		COMPILER_NO_REORDER(optik_t version = lock;);
		curr = head;

		do {
			pred = curr;
			curr = curr->next;
		} while (likely(curr->key < key));

		UPDATE_TRY();
		if (curr->key == key) {
			return false;
		}
		newnode = allocate_ll_simple<K,V>(key,val,curr);
		if (!optik_trylock_version(&lock, version)) {
			DO_PAUSE();
			ll_simple_release(newnode);
			goto restart;
		}
#ifdef __tile__
		MEM_BARRIER;
#endif
		pred->next = newnode;
		optik_unlock(&lock);

		return true;
	}

	V remove(K key)
	{
		volatile ll_simple<K,V> *pred, *curr;
		V result = (V) 0;
		NUM_RETRIES();

restart:
		COMPILER_NO_REORDER(optik_t version = lock;);
		PARSE_TRY();
		curr = head;
		do {
			pred = curr;
			curr = curr->next;
		} while (likely(curr->key < key));

		UPDATE_TRY();
		if (curr->key != key) {
			return false;
		}

		if (unlikely(!optik_trylock_version(&lock, version))) {
			DO_PAUSE();
			goto restart;
		}

		pred->next = curr->next;
		optik_unlock(&lock);
		result = curr->val;
		ll_simple_release<K,V>(curr);

		return result;
	}

	int length()
	{
		int count = 0;
		volatile ll_simple<K,V> *tmp;

		tmp = head->next;
		while(tmp->next != NULL) {
			count++;
			tmp = tmp->next;
		}

		return count;
	}
private:
	volatile ll_simple<K,V> *head;
	optik_t lock;
};

#endif
