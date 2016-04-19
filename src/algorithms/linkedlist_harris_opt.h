#ifndef _LINKEDLIST_HARRIS_OPT_H_
#define _LINKEDLIST_HARRIS_OPT_H_

extern "C" {
#include<stdlib.h>
#include "ssmem.h"
#include "atomic_ops.h"
}

#include "search.h"
#include "key_max_min.h"
#include "linklist_node_linked.h"

#define LOCKFREE

template<typename K, typename V,
	typename K_MAX_MIN = KeyMaxMin<K> >
class LinkedListHarrisOpt: public Search<K,V>
{
private:
	volatile node_ll_linked<K,V> *head;
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
	
	inline int physical_delete_right(volatile node_ll_linked<K,V> *left_node,
			volatile node_ll_linked<K,V> *right_node)
	{
		volatile node_ll_linked<K,V> *new_next =
			(volatile node_ll_linked<K,V> *)
				get_unmarked_ref((long)right_node->next);
		volatile node_ll_linked<K,V> *res = CAS_PTR(&left_node->next,
				right_node, new_next);
		int removed = (res == right_node);
#if GC==1
		if (likely(removed)) {
			ssmem_free(alloc, (void*) res);
		}
#endif
		return removed;
	}

	inline volatile node_ll_linked<K,V> *list_search(K key,
			volatile node_ll_linked<K,V> **left_node_ptr)
	{
		PARSE_TRY();
		volatile node_ll_linked<K,V> *left_node = head;
		volatile node_ll_linked<K,V> *right_node = head->next;
		while (1) {
			if (likely(!is_marked_ref((long)right_node->next))) {
				if (unlikely(right_node->key >= key)) {
					break;
				}
				left_node = right_node;
			} else {
				CLEANUP_TRY();
				physical_delete_right(left_node, right_node);
			}
			right_node = (volatile node_ll_linked<K,V> *)
				get_unmarked_ref((long)right_node->next);
		}
		*left_node_ptr = left_node;
		return right_node;
	}

	V harris_find(K key)
	{
		volatile node_ll_linked<K,V> *node = head->next;
		PARSE_TRY();

		while (likely(node->key < key)) {
			node = (volatile node_ll_linked<K,V> *)
				get_unmarked_ref((long)node->next);
		}

		if (node->key == key && !is_marked_ref((long)node->next)) {
			return node->val;
		}
		return 0;
	}

	int harris_insert(K key, V val)
	{
		do {
			UPDATE_TRY();
			volatile node_ll_linked<K,V> *left_node;
			volatile node_ll_linked<K,V> *right_node = list_search(
					key, &left_node);
			if (right_node->key == key) {
				return 0;
			}
			volatile node_ll_linked<K,V> *node_to_add =
				allocate_node_ll_linked(key, val, right_node);
#ifdef __tile__
			MEM_BARRIER;
#endif
			if (CAS_PTR(&left_node->next, right_node, node_to_add)
					== right_node) {
				return 1;
			}
#if GC==1
			ssmem_free(alloc, (void*) node_to_add);
#endif
		} while(1);
	}

	V harris_remove(K key)
	{
		volatile node_ll_linked<K,V> *cas_result;
		volatile node_ll_linked<K,V> *unmarked_ref;
		volatile node_ll_linked<K,V> *left_node;
		volatile node_ll_linked<K,V> *right_node;

		do {
			UPDATE_TRY();
			right_node = list_search(key, &left_node);
			if (right_node->key != key) {
				return 0;
			}
			unmarked_ref = (volatile node_ll_linked<K,V> *)
				get_unmarked_ref((long)right_node->next);
			volatile node_ll_linked<K,V> *marked_ref = 
				(volatile node_ll_linked<K,V> *)
						get_marked_ref((long)unmarked_ref);
			cas_result = (volatile node_ll_linked<K,V> *)
				CAS_PTR(&right_node->next, unmarked_ref,
						marked_ref);
		} while(cas_result != unmarked_ref);

		V ret = right_node->val;
		physical_delete_right(left_node, right_node);

		return ret;
	}

public:
	LinkedListHarrisOpt()
	{
		volatile node_ll_linked<K,V> *min, *max;

		head = (volatile node_ll_linked<K,V> *)
			malloc(sizeof(node_ll_linked<K,V>));
		if (NULL == head) {
			perror("malloc at LinkedListHarris constructor");
			exit(1);
		}
		max = initialize_node_ll_linked<K,V>(
				K_MAX_MIN::max_value(), 0, NULL);
		min = initialize_node_ll_linked<K,V>(
				K_MAX_MIN::min_value(), 0, max);
		head = min;
	}
	~LinkedListHarrisOpt()
	{
		volatile node_ll_linked<K,V> *node, *next;
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
#ifdef SEQUENTIAL
		volatile node_ll_linked<K,V> *prev, *next;
		prev = head;
		next = prev->next;
		while (next->key < key) {
			prev = next;
			next = prev->next;
		}
		result = (next->key == key) ? next->val : 0;
#elif defined LOCKFREE
		result = harris_find(key);
#endif
		return result;
	}

	int insert(K key, V val)
	{
		int success = 0;
#ifdef SEQUENTIAL
		volatile node_ll_linked<K,V> *prev, *next;
		prev = head;
		next = prev->head;
		while (next->key < key) {
			prev = next;
			next = prev->next;
		}
		success = (next->key != key);
		if (success) {
			prev->next = allocate_node_ll_linked<K,V>(key, val, next);
		}
#elif defined LOCKFREE
		success = harris_insert(key,val);
#endif
		return success;
	}

	V remove(K key)
	{
		V result = 0;

#ifdef SEQUENTIAL
		volatile node_ll_linked<K,V> *prev, *next;
		prev = head;
		next = prev->next;
		while (next->key < key) {
			prev = next;
			next = prev->next;
		}
		result = (next->key == key) ? next->val : 0;
		if (result) {
			prev->next = next->next;
			free(next);
		}
#elif defined LOCKFREE
		result = harris_remove(key);
#endif
		return result;
	}

	int length()
	{
		int size = 0;
		volatile node_ll_linked<K,V> *node;
		node = (volatile node_ll_linked<K,V> *)
			get_unmarked_ref((long)head->next);
		while ((volatile node_ll_linked<K,V> *)
				get_unmarked_ref((long)node->next) != NULL) {
			if (!is_marked_ref((long)node->next))
				size++;
			node = (volatile node_ll_linked<K,V> *)
				get_unmarked_ref((long)node->next);
		}
		return size;
	}

};
#endif
