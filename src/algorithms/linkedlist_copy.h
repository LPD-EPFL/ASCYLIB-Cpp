#ifndef __LINKED_LIST_COPY_H__
#define __LINKED_LIST_COPY_H__

extern "C" {
#include<assert.h>
#include<pthread.h>
#include<stdlib.h>
#include"lock_if.h"
#include"ssmem.h"
}
#include"search.h"
#include"linklist_node_simple.h"
#define DEFAULT_ALTERNATE               0
#define DEFAULT_EFFECTIVE               1

#define CPY_ON_WRITE_READ_ONLY_FAIL     RO_FAIL
#define CPY_ON_WRITE_USE_MEM_RELEAS     1

__thread ssmem_allocator_t* alloc;

template <typename K, typename V>
class LinkedListCopy : public Search<K,V> {
	public:
	LinkedListCopy()
	{
		//cow = (copy_on_write_t*)ssalloc_aligned(CACHE_LINE_SIZE, sizeof(copy_on_write_t));
		cow = (copy_on_write_t*)malloc(sizeof(copy_on_write_t));
		// MAYBE not use this allocator any more
		// TODO why align if ssaloc_aligned?
		assert(cow != NULL);
		//cow->lock = (ptlock_t*)ssalloc_aligned(CACHE_LINE_SIZE, sizeof(ptlock_t));
		cow->lock = (ptlock_t*)malloc(sizeof(ptlock_t));
		assert(cow->lock != NULL);

		INIT_LOCK_A(cow->lock);

		cow->array = initialize_ll_array<K,V>(0); //array_ll_new_init(0);
		//return cow;
	}
	//copy_on_write_t* copy_on_write_new(); // OVERLOAD New
	//size_t size();
	V search(K key)
	{
		volatile ll_array<K,V>* current_list = cow->array;
		for (int i=0; i < current_list->size; i++) {
			if ( /*unlikely*/ current_list->nodes[i].key == key) {
				return current_list->nodes[i].val;
			}
		}
		return (V)0;
	}
	int insert(K key, V value)
	{
#if CPY_ON_WRITE_READ_ONLY_FILE == 1
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
			if (/*unlikely*/ all_old->nodes[i].key == key) {
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
#if CPY_ON_WRITE_READ_ONLY_FILE == 1
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
			if ( /*unlikely */ all_old->nodes[i].key == key ) {
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

		#  if CPY_ON_WRITE_USE_MEM_RELEAS == 1
			SSMEM_SAFE_TO_RECLAIM();
			ssmem_release(alloc, (void*) a);
		#  else
			ssmem_free(alloc, (void*) a);
		#  endif

		#endif
		}

		/*
		inline volatile array_ll_t* array_ll_new_init(size_t size)
		{
			array_ll_t* all;
			all = (array_ll_t*) memalign(CACHE_LINE_SIZE,
				sizeof(array_ll_t) +
				(array_ll_fixed_size * sizeof(key_value_t)) );
			assert(all != NULL);

			all->size = size;
			all->kvs = (key_value_t*)((uintptr_t)all + sizeof(array_ll_t));

			return all;
		}
		inline array_ll_t* array_ll_new(size_t size)
		{
			array_ll_t* all;
		#if GC == 1
			all = (array_ll_t*)malloc(sizeof(array_ll_t) +
				(array_ll_fixed_size * sizeof(key_value_t)));
		#else
			all = (array_ll_t*)ssalloc(sizeof(array_ll_t) +
				(array_ll_fixed_size * sizeof(key_value_t)));
		#endif
			assert(all != NULL);

			all->size = size;
			all->kvs = (key_value_t*)((uintptr_t)all + sizeof(array_ll_t));

			return all;
		}
*/

};

/*
copy_on_write_t* copy_on_write_new();
size_t copy_on_write_size(copy_on_write_t* set);
sval_t cpy_search(copy_on_write_t* set, skey_t key);
int cpy_insert(copy_on_write_t* set, skey_t key, sval_t val);
sval_t cpy_delete(copy_on_write_t* set, skey_t key);
*/

// TODO create memory allocation interface ASCYLIB_malloc ASYCLIB_FREE
// free with GC and without GC
// TODO externalize "alloc" to memory allocator class?

#endif
