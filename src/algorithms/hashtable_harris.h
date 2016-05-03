#ifndef _HASHTABLE_HARRIS_H_
#define _HASHTABLE_HARRIS_H_

#define DEFAULT_LOAD	1

#include "search.h"
#include "key_hasher.h"
#include "linkedlist_harris_opt.h"

template<typename K, typename V,
	typename KEY_HASHER = KeyHasher<K> >
class HashtableHarris : public Search<K,V>
{

public:
	V search(K key)
	{
		int addr = KEY_HASHER::hash(key) & hash;
		return buckets[addr]->search(key);
	}

	int insert(K key, V val)
	{
		int addr = KEY_HASHER::hash(key) & hash;
		return buckets[addr]->insert(key, val);
	}

	V remove(K key)
	{
		int addr = KEY_HASHER::hash(key) & hash;
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

	HashtableHarris(int max_ht_length)
	{
		// TODO ensure that maxhtlength is a power of 2?
		maxhtlength = max_ht_length;
		hash = maxhtlength - 1;
		buckets = new LinkedListHarrisOpt<K,V>*[maxhtlength];
		for(int i=0; i<maxhtlength; i++) {
			buckets[i] = new LinkedListHarrisOpt<K,V>();
		}
	}

	~HashtableHarris()
	{
		for(int i=0; i<maxhtlength; i++) {
			delete buckets[i];
		}
		delete buckets;
	}
private:
	unsigned int maxhtlength;
	size_t hash;
	LinkedListHarrisOpt<K,V> **buckets;
};

#endif
