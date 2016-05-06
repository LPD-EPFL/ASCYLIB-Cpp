#ifndef _LINKED_LIST_OPTIK_H_
#define LINKED_LIST_OPTIK_H_

extern "C" {
#include "optik.h"
#include "utils.h"
}
#include "search.h"
#include "key_max_min.h"
#include "ll_optik.h"

template <typename K, typename V,
	 typename K_MAX_MIN = KeyMaxMin<K> >
class LinkedListOptik : public Search<K,V>
{
public:
	LinkedListOptik()
	{
		volatile ll_optik<K,V> *min, *max;
		max = initialize_ll_optik<K,V>(K_MAX_MIN::max_value(),
			(V) 0, (volatile ll_optik<K,V> *)NULL);
		min = initialize_ll_optik<K,V>(K_MAX_MIN::min_value(),
			(V) 0, max);
		head = min;

		MEM_BARRIER;
	}

	~LinkedListOptik()
	{
		volatile ll_optik<K,V> *curr, *next;
		curr = head;
		while (NULL != curr) {
			next = curr->next;
			ll_optik_delete<K,V>( curr );
			curr = next;
		}
	}

	V search(K key)
	{
		PARSE_TRY();
		volatile ll_optik<K,V> *curr = head;

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
		optik_t pred_ver = OPTIK_INIT;
restart:
		PARSE_TRY();
		volatile ll_optik<K,V> *curr, *pred;
		curr = head;
		do {
			COMPILER_NO_REORDER(optik_t curr_ver = curr->lock;);
			pred = curr;
			pred_ver = curr_ver;
			curr = curr->next;
		} while (likely(curr->key < key));

		UPDATE_TRY();
		if (curr->key == key) {
			return false;
		}
		volatile ll_optik<K,V> *newnode =
			allocate_ll_optik<K,V>(key, val, curr);
		if (!optik_trylock_version(&pred->lock, pred_ver)) {
			ll_optik_release(newnode);
			goto restart;
		}
#ifdef __tile__
		MEM_BARRIER;
#endif
		pred->next = newnode;
		optik_unlock(&pred->lock);

		return true;
	}

	V remove(K key)
	{
		optik_t pred_ver = OPTIK_INIT, curr_ver = OPTIK_INIT;
restart:
		PARSE_TRY();
		volatile ll_optik<K,V> *pred, *curr;
		curr = head;
		curr_ver = curr->lock;
		do {
			pred = curr;
			pred_ver = curr_ver;

			curr = curr->next;
			curr_ver = curr->lock;
		} while (likely(curr->key < key));

		UPDATE_TRY();
		if (curr->key != key) {
			return false;
		}

		volatile ll_optik<K,V>* cnxt = curr->next;
		if (unlikely(!optik_trylock_version(&pred->lock, pred_ver))) {
			goto restart;
		}
		if (unlikely(!optik_trylock_version(&curr->lock, curr_ver))) {
			optik_revert(&pred->lock);
			goto restart;
		}

		pred->next = cnxt;
		optik_unlock(&pred->lock);

		V result = curr->val;
		ll_optik_release<K,V>(curr);

		return result;
	}

	int length()
	{
		int count = 0;
		volatile ll_optik<K,V> *tmp;

		tmp = head->next;
		while(tmp->next != NULL) {
			count++;
			tmp = tmp->next;
		}

		return count;
	}
private:
	volatile ll_optik<K,V> *head;
};

#endif
