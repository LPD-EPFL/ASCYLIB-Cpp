#ifndef _QUEUE_MS_LB_H_
#define _QUEUE_MS_LB_H_

#include<stdlib.h>
#include"stack_queue.h"
#include<atomic_ops.h>
#include"lock_if.h"
#include"optik.h"
#include"stackqueue_node.h"

template <typename T>
class QueueMSLB : public StackQueue<T>
{
public:
	QueueMSLB()
	{
		set = (queue*) aligned_alloc(CACHE_LINE_SIZE, sizeof(queue));
		if (NULL == set) {
			perror("QueueMSLB");
			exit(1);
		}
		stackqueue_node<T>* node = (stackqueue_node<T>*)
			aligned_alloc(CACHE_LINE_SIZE, sizeof(stackqueue_node<T>) );
		node->next = NULL;
		set->head = node;
		set->tail = node;
	}

	~QueueMSLB()
	{
		// TODO implement me, not implemented in C either
	}

	int add(T item)
	{
		stackqueue_node<T>* node = allocate_stackqueue_node(item,
				(stackqueue_node<T>*) NULL);
		LOCK_A(&set->tail_lock);
		set->tail->next = node;
		set->tail = node;
		UNLOCK_A(&set->tail_lock);
		return 1;
	}

	T remove()
	{
		T val = 0;
		LOCK_A(&set->head_lock);
		stackqueue_node<T>* node = set->head;
		stackqueue_node<T>* head_new = node->next;
		if (head_new == NULL) {
			UNLOCK(&set->head_lock);
			return 0;
		}
		val = head_new->item;
		set->head = head_new;
		UNLOCK_A(&set->head_lock);
		release_stackqueue_node(node);
		return val;
	}

	int count()
	{
		int count = 0;
		stackqueue_node<T> *node;

		node = set->head;
		while (node->next != NULL) {
			count++;
			node = node->next;
		}
		return count;
	}

private:
	struct ALIGNED(CACHE_LINE_SIZE) queue
	{
		union {
			struct {
				stackqueue_node<T> *head;
				ptlock_t head_lock;
			};
			uint8_t padding1[CACHE_LINE_SIZE];
		};
		union {
			struct {
				stackqueue_node<T> *tail;
				ptlock_t tail_lock;
			};
			uint8_t padding2[CACHE_LINE_SIZE];
		};
	};
	queue *set;
};

#endif
