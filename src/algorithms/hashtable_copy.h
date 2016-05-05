#ifndef _HASH_TABLE_COPY_H_
#define _HASH_TABLE_COPY_H_

#include<assert.h>
#include<stdlib.h>
extern "C" {
#include"lock_if.h"
#include"ssmem.h"
#include"utils.h"
}
#include"search.h"
#include"ll_array.h"
#include"linkedlist_copy.h"
#define DEFAULT_ALTERNATE               0
#define DEFAULT_EFFECTIVE               1
#define DEFAULT_LOAD                    1

#define CPY_ON_WRITE_READ_ONLY_FAIL     RO_FAIL

extern __thread ssmem_allocator_t* alloc;

template <typename K, typename V,
	 typename KEY_HASHER = KeyHasher<K> >
class HashtableCopy : public Search<K,V>
{
	public:
	HashtableCopy(size_t num_buckets)
	{
		this->num_buckets = num_buckets;
		hash = num_buckets - 1;
		buckets = new LinkedListCopy<K,V>*[num_buckets];
		for (int i=0; i<num_buckets; i++) {
			buckets[i] = new LinkedListCopy<K,V>();
		}
	}

	~HashtableCopy()
	{
		for (int i=0; i<num_buckets;i++) {
			delete buckets[i];
		}
		delete buckets;
	}

	V search(K key)
	{
		int addr = KEY_HASHER::hash(key) & hash;
		return buckets[addr]->search(key);
	}
	int insert(K key, V value)
	{
		int addr = KEY_HASHER::hash(key) & hash;
		return buckets[addr]->insert(key,value);
	}

	V remove(K key)
	{
		int addr = KEY_HASHER::hash(key) & hash;
		return buckets[addr]->remove(key);
	}

	int length()
	{
		int count = 0;
		for (int i=0;i<num_buckets;i++) {
			count += buckets[i]->length();
		}
		return count;
	}

	private:
		size_t num_buckets;
		size_t hash;
		LinkedListCopy<K,V> **buckets;
};
#endif
