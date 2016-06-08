#ifndef _STACK_TREIBER_H_
#define _STACK_TREIBER_H_

#include "stack_queue.h"
#include "stackqueue_node_padded.h"
#include <stdlib.h>

template <typename T>
class StackTreiber : public StackQueue<T>
{
public:
	StackTreiber()
	{
		set = (stack*) aligned_alloc(CACHE_LINE_SIZE, sizeof(stack));
		if (NULL == set) {
			perror("StackTreiber");
			exit(1);
		}
		set->top = NULL;
	}

	~StackTreiber()
	{
		// TODO implement me, not implemented in C either
	}

	int add(T item)
	{
		NUM_RETRIES();
		stackqueue_node_padded<T> *node;
		node = allocate_stackqueue_node_padded(item,
				(stackqueue_node_padded<T>*) NULL);
		while (1) {
			stackqueue_node_padded<T> *top = set->top;
			node->next = top;
			if (CAS_PTR(&set->top, top, node) == top) {
				break;
			}
			DO_PAUSE();
		}
		return 1;
	}

	T remove()
	{
		stackqueue_node_padded<T> *top;
		NUM_RETRIES();
		while (1) {
			top = set->top;
			if (unlikely(NULL == top)) {
				return 0;
			}
			if (CAS_PTR(&set->top, top, top->next) == top) {
				break;
			}
			DO_PAUSE();
		}
		T val = top->item;
		release_stackqueue_node_padded( top );
		return val;
	}

	int count()
	{
		int count = 0;
		stackqueue_node_padded<T> *tmp;
		tmp = set->top;
		while (tmp != NULL) {
			count++;
			tmp = tmp->next;
		}
		return count;
	}

private:
	struct ALIGNED(CACHE_LINE_SIZE) stack {
		union {
			stackqueue_node_padded<T> *top;
			uint8_t padding[CACHE_LINE_SIZE];
		};
	};
	stack *set;
};

#endif
