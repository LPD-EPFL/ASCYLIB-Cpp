#ifndef _HASHTABLE_OPTIK_H_
#define _HASHTABLE_OPTIK_H_

#define DEFAULT_MOVE                    0
#define DEFAULT_SNAPSHOT                0
#define DEFAULT_LOAD                    1
#define DEFAULT_ELASTICITY              1
#define DEFAULT_ALTERNATE               0
#define DEFAULT_EFFECTIVE               1

#define MAXHTLENGTH                     65536

#include "search.h"
#include "key_hasher.h"
#include "linkedlist_optik_gl.h"

template<typename K, typename V,
	typename KEY_HASHER = KeyHasher<K> >
class HashtableOptik : public Search<K,V>
{
public:
	HashtableOptik(int max_ht_length)
	{
		maxhtlength = max_ht_length;
		hash = maxhtlength - 1;
		buckets = new LinkedListOptikGL<K,V>*[maxhtlength];
		for (int i=0; i<maxhtlength; i++) {
			buckets[i] = new LinkedListOptikGL<K,V>();
		}
	}

	~HashtableOptik()
	{
		for (int i=0; i<maxhtlength; i++) {
			delete buckets[i];
		}
		delete buckets;
	}

	V search(K key)
	{
		int addr = KEY_HASHER::hash(key) & hash;
		return buckets[addr]->search(key);
	}

	int insert(K key, V val)
	{
		int addr = KEY_HASHER::hash(key) & hash;
		return buckets[addr]->insert(key,val);
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
private:
	int maxhtlength;
	size_t hash;
	LinkedListOptikGL<K,V>** buckets;
};

#endif
