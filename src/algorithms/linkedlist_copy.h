#ifndef _LINKED_LIST_COPY_H_
#define _LINKED_LIST_COPY_H_

#include<assert.h>
#include<pthread.h>
#include<stdlib.h>
extern "C" {
#include"lock_if.h"
#include"ssmem.h"
#include"utils.h"
}
#include"search.h"
#include"ll_array.h"
#define DEFAULT_ALTERNATE               0
#define DEFAULT_EFFECTIVE               1

#define CPY_ON_WRITE_READ_ONLY_FAIL     RO_FAIL
#define CPY_ON_WRITE_USE_MEM_RELEAS     1

extern __thread ssmem_allocator_t* alloc;

template <typename K, typename V>
class LinkedListCopy : public Search<K,V>
{
	public:
	LinkedListCopy()
	{
		cow = (copy_on_write_t*)malloc(sizeof(copy_on_write_t));
		assert(cow != NULL);
		cow->lock = (ptlock_t*)malloc(sizeof(ptlock_t));
		assert(cow->lock != NULL);

		INIT_LOCK_A(cow->lock);

		cow->array = initialize_ll_array<K,V>(0);
	}

	V search(K key)
	{
		volatile ll_array<K,V>* current_list = cow->array;
		for (int i=0; i < current_list->size; i++) {
			if (unlikely(current_list->nodes[i].key == key)) {
				return current_list->nodes[i].val;
			}
		}
		return (V)0;
	}
	int insert(K key, V value)
	{
#if CPY_ON_WRITE_READ_ONLY_FAIL == 1
		if (search(key) == 0) {
			return 0;
		}
#endif
		LOCK_A( cow->lock );
		volatile ll_array<K,V>* all_old = cow->array;
		volatile ll_array<K,V>* all_new =
			allocate_ll_array<K,V>(all_old->size + 1);

		int i;
		for(i=0; i < all_old->size; i++) {
			if (unlikely(all_old->nodes[i].key == key)) {
				UNLOCK_A( cow->lock );
				cpy_delete_copy(alloc, all_new);
				return 0;
			}
			all_new->nodes[i].key = all_old->nodes[i].key;
			all_new->nodes[i].val = all_old->nodes[i].val;
		}
		all_new->nodes[i].key = key;
		all_new->nodes[i].val = value;

#ifdef __tile__
		MEM_BARRIER;
#endif
		cow->array = all_new;
		UNLOCK_A( cow->lock );
		cpy_delete_copy(alloc, all_old);
		return 1;
	}

	V remove(K key)
	{
#if CPY_ON_WRITE_READ_ONLY_FAIL == 1
		if (search(key) == 0) {
			return 0;
		}
#endif
		V removed = 0;

		LOCK_A( cow->lock );

		volatile ll_array<K,V>* all_old = cow->array;
		volatile ll_array<K,V>* all_new =
			allocate_ll_array<K,V>(all_old->size - 1);

		int i,n;
		for(i=0,n=0; i < all_old->size; i++,n++) {
			if ( unlikely( all_old->nodes[i].key == key ) ) {
				removed = all_old->nodes[i].val;
				n--;
				continue;
			}
			all_new->nodes[n].key = all_old->nodes[i].key;
			all_new->nodes[n].val = all_old->nodes[i].val;
		}

#ifdef __tile__
		MEM_BARRIER;
#endif
		volatile ll_array<K,V>* to_delete = all_new;
		if (removed) {
			cow->array = all_new;
			to_delete = all_old;
		}
		UNLOCK_A(cow->lock);
		cpy_delete_copy(alloc, (volatile ll_array<K,V> *)to_delete);
		return removed;
	}

	int length()
	{
		return cow->array->size;
	}

	private:
		typedef /*ALIGN but maybe not necessary*/ struct copy_on_write
		{
			union
			{
				struct
				{
					volatile ptlock_t* lock;
					volatile ll_array<K,V>* array;
				};
				uint8_t padding[CACHE_LINE_SIZE];
			};
		} copy_on_write_t;
		copy_on_write_t* cow;
		size_t array_ll_fixed_size;

		inline void cpy_delete_copy(ssmem_allocator_t* alloc,
				volatile ll_array<K,V>* a)
		{
#if GC == 1

#	if CPY_ON_WRITE_USE_MEM_RELEAS == 1
			SSMEM_SAFE_TO_RECLAIM();
			ssmem_release(alloc, (void*) a);
#	else
			ssmem_free(alloc, (void*) a);
#	endif

#endif
		}
};
#endif
