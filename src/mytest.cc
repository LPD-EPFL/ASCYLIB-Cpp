#include<pthread.h>
#include"common.h"
#include"utils.h"
#include"search.h"
#include"linkedlist_lazy.h"

int main()
{
#if GC == 1
        alloc = (ssmem_allocator_t*) malloc(sizeof(ssmem_allocator_t));
        assert(alloc != NULL);
        ssmem_alloc_init_fs_size(alloc, SSMEM_DEFAULT_MEM_SIZE, SSMEM_GC_FREE_SET_SIZE, ID);
#endif

	Search<int, int> *set = (Search <int,int> *)new LinkedListLazy<int, int>();

	set->insert(5,6);
	set->insert(177,7);
	printf("At key 5, I find %i\n", set->search(5));
	printf("At key 177, I find %i\n", set->search(177));
	printf("The data structure has %i elements\n", set->length());
	set->remove(177);
	printf("At key 177, I find %i\n", set->search(177));
	printf("The data structure has %i elements\n", set->length());
	return 0;
}
