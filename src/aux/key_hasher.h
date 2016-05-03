#ifndef _KEY_HASHER_H_
#define _KEY_HASHER_H_

template <typename K>
class KeyHasher
{
public:
	static int hash(K key)
	{
		return (int) key;
	}
};

template <>
class KeyHasher<int>
{
public:
	static int hash(int key)
	{
		return key;
	}
};
#endif
