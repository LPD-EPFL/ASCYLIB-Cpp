#ifndef _HASHTABLE_PUGH_H_
#define _HASHTABLE_PUGH_H_

#include<stdio.h>
#include<stdlib.h>

#include"search.h"
#include"key_hasher.h"
#include"linkedlist_pugh.h"

#define DEFAULT_MOVE                    0
#define DEFAULT_SNAPSHOT                0
#define DEFAULT_LOAD                    1
#define DEFAULT_ELASTICITY              1
#define DEFAULT_ALTERNATE               0
#define DEFAULT_EFFECTIVE               1

#define MAXHTLENGTH                     65536

template <typename K, typename V,
	 typename KEY_HASHER = KeyHasher<K> >
class HashtablePugh : public Search<K,V>
{
public:
	HashtablePugh(int max_ht_length)
	{
		maxhtlength = max_ht_length;
		hash = maxhtlength - 1;
		buckets = new LinkedListPugh<K,V>*[maxhtlength];
		for (int i=0; i<maxhtlength; i++) {
			buckets[i] = new LinkedListPugh<K,V>();
		}
	}

	~HashtablePugh()
	{
		for (int i=0; i<maxhtlength; i++) {
			delete buckets[i];
		}
		delete buckets;
	}

	V search(K key)
	{
		int addr = key & hash;
		return buckets[addr]->search(key);
	}

	int insert(K key, V value)
	{
		int addr = key & hash;
		return buckets[addr]->insert(key, value);
	}

	V remove(K key)
	{
		int addr = key & hash;
		return buckets[addr]->remove(key);
	}

	int length()
	{
		int count = 0;
		for (int i=0; i<maxhtlength; i++) {
			count += buckets[i]->length();
		}
		return count;
	}
private:
	int maxhtlength;
	size_t hash;
	LinkedListPugh<K,V>** buckets;
};

#endif
