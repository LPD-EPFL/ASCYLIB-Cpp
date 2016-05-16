#ifndef _ARRAYMAP_OPTIK_H_
#define _ARRAYMAP_OPTIK_H_

#include<stdlib.h>
#include<malloc.h>
#include<assert.h>
#include"optik.h"
#include"search.h"

template <typename K, typename V>
class ArrayMapOptik : public Search<K,V>
{
public:
	ArrayMapOptik(int size)
	{
		map = (volatile ll_array_optik*) malloc(sizeof(ll_array_optik));
		assert(map != NULL);

		map->size = size;
		map->array = (key_val*) malloc( size * sizeof(key_val));
		assert(map->array != NULL);
		memset(map->array, 0 , size * sizeof(key_val));

		map->lock = (volatile optik_t*) malloc(sizeof(optik_t));
		if (map->lock == NULL) {
			perror("malloc @ allocate_ll_array_optik");
			exit(1);
		}
		optik_init(map->lock);
		MEM_BARRIER;
	}

	~ArrayMapOptik()
	{
		free(map->array);
		free(map->lock);
		free(map);
	}

	V search(K key)
	{
restart:
		COMPILER_NO_REORDER(optik_t version = optik_get_version_wait(map->lock););
		for (int i=0; i < map->size; i++) {
			if (map->array[i].key == key) {
				V val = map->array[i].val;
				if (optik_is_same_version(version, *map->lock)) {
					return val;
				}
				goto restart;
			}
		}
		return 0;
	}

	int insert(K key, V val)
	{
		NUM_RETRIES();
		optik_t version;
restart:
		COMPILER_NO_REORDER(version = *map->lock;);
		int free_idx = -1;
		for (int i=0;i < map->size; i++) {
			K ck = map->array[i].key;
			if (ck == key) {
				return 0;
			} else if (ck == 0) {
				free_idx = i;
			}
		}
		if (!optik_trylock_version(map->lock, version)) {
			DO_PAUSE();
			goto restart;
		}
		int res = 0;
		if (free_idx >= 0) {
			map->array[free_idx].key = key;
			map->array[free_idx].val = val;
			res = 1;
		}
		optik_unlock(map->lock);
		return res;
	}

	V remove(K key)
	{
		NUM_RETRIES();
		optik_t version;
restart:
		COMPILER_NO_REORDER(version = *map->lock;);
		for (int i=0; i < map->size; i++) {
			if (map->array[i].key == key) {
				if (!optik_trylock_version(map->lock, version)) {
					DO_PAUSE();
					goto restart;
				}
				V val = map->array[i].val;
				map->array[i].key = 0;
				optik_unlock(map->lock);
				return val;
			}
		}
		return 0;
	}

	int length()
	{
		int count = 0;
		for (int i=0; i<map->size; i++) {
			count += (map->array[i].key != 0);
		}
		return count;
	}
private:
	struct key_val {
		K key;
		V val;
	};

	struct ll_array_optik {
		union {
			struct {
				key_val *array;
				size_t size;
				optik_t* lock;
			};
			uint8_t padding[CACHE_LINE_SIZE];
		};
	};

	volatile ll_array_optik *map;
};
#endif
