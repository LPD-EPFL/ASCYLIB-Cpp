#ifndef _STACK_LOCK_H_
#define _STACK_LOCK_H_

#include<stdlib.h>
#include"stackqueue_node.h"
#include"stack_queue.h"
#include"lock_if.h"
#include"optik.h"

template <typename T>
class StackLock : public StackQueue<T>
{
public:
	StackLock()
	{
		set = (stack*) aligned_alloc(CACHE_LINE_SIZE, sizeof(stack));
		if (NULL == set) {
			perror("StackLock");
			exit(1);
		}
		set->top = NULL;
	}

	~StackLock()
	{
		// TODO not implemented in C either
	}

	int add(T item)
	{
		stackqueue_node<T> *node;
		node = allocate_stackqueue_node(item,
				(stackqueue_node<T>*) NULL);
		LOCK_A(&set->lock);
		node->next = set->top;
		set->top = node;
		UNLOCK_A(&set->lock);
		return 1;
	}

	T remove()
	{
		LOCK_A(&set->lock);
		stackqueue_node<T> *top;
		top = set->top;
		if (unlikely(NULL == top))
		{
			UNLOCK_A(&set->lock);
			return 0;
		}
		set->top = top->next;
		UNLOCK_A(&set->lock);
		T val = top->item;
		release_stackqueue_node(top);
		return val;
	}

	int count()
	{
		int count = 0;
		stackqueue_node<T> *tmp;
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
			struct {
				stackqueue_node<T>* top;
				ptlock_t lock;
			};
			uint8_t padding[CACHE_LINE_SIZE];
		};
	};
	stack *set;
};

#endif
