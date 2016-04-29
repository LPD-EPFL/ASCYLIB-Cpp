#ifndef _LINKEDLIST_HARRIS_H_
#define _LINKEDLIST_HARRIS_H_

#include<stdlib.h>
extern "C" {
#include "ssmem.h"
#include "atomic_ops.h"
}

#include "search.h"
#include "key_max_min.h"
#include "ll_simple.h"

template<typename K, typename V,
	typename K_MAX_MIN = KeyMaxMin<K> >
class LinkedListHarris: public Search<K,V>
{
private:
	volatile ll_simple<K,V> *head;
	/*
	 * The five following functions handle the low-order mark bit that indicates
	 * whether a node is logically deleted (1) or not (0).
	 *  - is_marked_ref returns whether it is marked,
	 *  - (un)set_marked changes the mark,
	 *  - get_(un)marked_ref sets the mark before returning the node.
	 */
	inline int is_marked_ref(long i)
	{
		/* return (int) (i & (LONG_MIN+1)); */
		return (int) (i & 0x1L);
	}

	inline long unset_mark(long i)
	{
		/* i &= LONG_MAX-1; */
		i &= ~0x1L;
		return i;
	}

	inline long set_mark(long i)
	{
		/* i = unset_mark(i); */
		/* i += 1; */
		i |= 0x1L;
		return i;
	}

	inline long get_unmarked_ref(long w)
	{
		/* return unset_mark(w); */
		return w & ~0x1L;
	}

	inline long get_marked_ref(long w)
	{
		/* return set_mark(w); */
		return w | 0x1L;
	}

	volatile ll_simple<K,V> *harris_search(K key,
			volatile ll_simple<K,V> **left_node)
	{
		volatile ll_simple<K,V> *left_node_next, *right_node;
		left_node_next = head;

		do {
			PARSE_TRY();
			volatile ll_simple<K,V> *t = head;
			volatile ll_simple<K,V> *t_next = head->next;

			do {
				if (!is_marked_ref((long) t_next)) {
					(*left_node) = t;
					left_node_next = t_next;
				}
				t = (volatile ll_simple<K,V> *)
					get_unmarked_ref((long) t_next);
				if (!t->next) {
					break;
				}
				t_next = t->next;
			} while(is_marked_ref((long)t_next) || (t->key < key));
			right_node = t;

			if (left_node_next == right_node) {
				if (right_node->next && is_marked_ref(
							(long) right_node->next)) {
					continue;
				} else {
					return right_node;
				}
			}
			CLEANUP_TRY();
			if (ATOMIC_CAS_MB(&(*left_node)->next, left_node_next,
						right_node)) {
#if GC==1
				volatile ll_simple<K,V> *cur = left_node_next;
				do {
					volatile ll_simple<K,V>
						*node_to_free = cur;
					cur = (volatile ll_simple<K,V> *)
						get_unmarked_ref((long) cur->next);
					ssmem_free(alloc, (void*)node_to_free);
				} while (cur != right_node);
#endif
				if (!(right_node->next &&
					   is_marked_ref((long) right_node->next))) {
					return right_node;
				}
			}
		} while(1);
	}

	V harris_find(K key)
	{
		volatile ll_simple<K,V> *right_node, *left_node;
		left_node = head;

		right_node = harris_search(key, &left_node);
		if ((right_node->next == NULL) || (right_node->key != key)) {
			return 0;
		} else {
			return right_node->val;
		}

	}

	int harris_insert(K key, V val)
	{
		volatile ll_simple<K,V> *newnode = NULL,
			 *right_node, *left_node;
		left_node = head;

		do {
			UPDATE_TRY();
			right_node = harris_search(key, &left_node);
			if (right_node->key == key) {
#if GC == 1
				if (unlikely(newnode != NULL)) {
					ssmem_free(alloc, (void *)newnode);
				}
#endif
				return 0;
			}
			if (likely(newnode == NULL)) {
				newnode = allocate_ll_simple<K,V>(key, val,
						right_node);
			} else {
				newnode->next = right_node;
			}
#ifdef __tile__
			MEM_BARRIER;
#endif
			if (ATOMIC_CAS_MB(&left_node->next, right_node, newnode)) {
				return 1;
			}
		} while(1);
	}

	V harris_remove(K key)
	{
		volatile ll_simple<K,V> *right_node, *right_node_next,
			 *left_node;
		left_node = head;
		V ret = 0;
		do {
			UPDATE_TRY();
			right_node = harris_search(key, &left_node);
			if (right_node->key != key) {
				return 0;
			}
			right_node_next = right_node->next;
			if (!is_marked_ref((long)right_node_next)) {
				if (ATOMIC_CAS_MB(&right_node->next,
					right_node_next,
					get_marked_ref((long) right_node_next)))
				{
					ret = right_node->val;
					break;
				}
			}
		} while(1);
		if (likely(ATOMIC_CAS_MB(&left_node->next, right_node,
			right_node_next))) {
#if GC == 1
			ssmem_free(alloc,
				(void*) get_unmarked_ref((long) right_node));
#endif
		;
		} else {
			harris_search(key, &left_node);
		}

		return ret;

	}

public:
	LinkedListHarris()
	{
		volatile ll_simple<K,V> *min, *max;

		max = initialize_ll_simple<K,V>(
			K_MAX_MIN::max_value(), 0, NULL);
		min = initialize_ll_simple<K,V>(
			K_MAX_MIN::min_value(), 0, max);
		head = min;
		MEM_BARRIER;
	}

	~LinkedListHarris()
	{
		volatile ll_simple<K,V> *node, *next;
		node = head;

		while (node!=NULL) {
			next = node->next;
			free( (void*) node);
			node = next;
		}
	}

	V search(K key)
	{
		V result = (V)0;
		result = harris_find(key);
		return result;
	}

	int insert(K key, V val)
	{
		int success = 0;
		success = harris_insert(key,val);
		return success;
	}

	V remove(K key)
	{
		V result = 0;
		result = harris_remove(key);
		return result;
	}

	int length()
	{
		int size = 0;
		volatile ll_simple<K,V> *node;
		node = (volatile ll_simple<K,V> *)
			get_unmarked_ref((long)head->next);
		while ((volatile ll_simple<K,V> *)
				get_unmarked_ref((long)node->next) != NULL) {
			if (!is_marked_ref((long)node->next))
				size++;
			node = (volatile ll_simple<K,V> *)
				get_unmarked_ref((long)node->next);
		}
		return size;
	}

};
#endif
