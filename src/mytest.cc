#include<pthread.h>
#include"common.h"
#include"utils.h"
#include"search.h"
#include"ssmem.h"
#include"hashtable_java.h"

size_t initial = DEFAULT_INITIAL;
size_t range = DEFAULT_RANGE;
size_t load_factor = DEFAULT_LOAD;
size_t update = DEFAULT_UPDATE;
size_t num_threads = DEFAULT_NB_THREADS;
size_t duration = DEFAULT_DURATION;

size_t array_ll_fixed_size = DEFAULT_RANGE;
__thread ssmem_allocator_t* alloc;
__thread unsigned long *seeds;

int main()
{
	unsigned int maxhtlength = (unsigned int) initial / load_factor;
	unsigned int maxbulength = ((unsigned int) range / initial) * load_factor;
#if GC == 1
	int ID = 0;
        alloc = (ssmem_allocator_t*) malloc(sizeof(ssmem_allocator_t));
        assert(alloc != NULL);
        ssmem_alloc_init_fs_size(alloc, SSMEM_DEFAULT_MEM_SIZE, SSMEM_GC_FREE_SET_SIZE, ID);
#endif
	Search<int, int> *set = (Search <int,int> *)new HashtableJavaCHM<int, int>(maxhtlength,maxbulength);

	printf("STARTING TEST\n");
	printf("The result of the insertion is: %i\n",set->insert(5,6));
	printf("The result of the insertion is: %i\n",set->insert(177,7));
	printf("At key 5, I find %i\n", set->search(5));
	printf("At key 177, I find %i\n", set->search(177));
	printf("The data structure has %i elements\n", set->length());
	set->remove(177);
	printf("At key 177, I find %i\n", set->search(177));
	printf("The data structure has %i elements\n", set->length());
	return 0;
}
