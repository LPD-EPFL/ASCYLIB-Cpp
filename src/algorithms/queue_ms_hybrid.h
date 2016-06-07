#ifndef _QUEUE_MS_HYBRID_H_
#define _QUEUE_MS_HYBRID_H_

#include<stdlib.h>
#include"stack_queue.h"
#include<atomic_ops.h>
#include"lock_if.h"
#include"optik.h"
#include"queue_node.h"

template <typename T>
class QueueMSHybrid : public StackQueue<T>
{
public:
	QueueMSHybrid()
	{
		set = (queue*) aligned_alloc(CACHE_LINE_SIZE, sizeof(queue));
		if (NULL == set) {
			perror("QueueMSHybrid");
			exit(1);
		}
		queue_node<T>* node = (queue_node<T>*)
			aligned_alloc(CACHE_LINE_SIZE, sizeof(queue_node<T>) );
		node->next = NULL;
		set->head = node;
		set->tail = node;
	}

	~QueueMSHybrid()
	{
	}

	int add(T item)
	{
		queue_node<T>* node = allocate_queue_node(item,
				(queue_node<T>*) NULL);
		LOCK_A(&set->tail_lock);
		set->tail->next = node;
		set->tail = node;
		UNLOCK_A(&set->tail_lock);
		return 1;
	}

	T remove()
	{
		NUM_RETRIES();
		queue_node<T> *next, *head;
		while(1) {
			head = set->head;
			queue_node<T>* tail = set->tail;
			next = head->next;
			if (likely(head == set->head)) {
				if (head == tail) {
					if (NULL == next) {
						return 0;
					}
					UNUSED void* dummy = CAS_PTR(&set->tail,
							tail, next);
				} else {
					if (CAS_PTR(&set->head, head,next) == head) {
						break;
					}
				}
			}
			DO_PAUSE();
		}
		T item = next->item;
		release_queue_node(head);
		return item;
	}

	int count()
	{
		int count = 0;
		queue_node<T> *node;

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
				queue_node<T> *head;
				ptlock_t head_lock;
			};
			uint8_t padding1[CACHE_LINE_SIZE];
		};
		union {
			struct {
				queue_node<T> *tail;
				ptlock_t tail_lock;
			};
			uint8_t padding2[CACHE_LINE_SIZE];
		};
	};
	queue *set;
};

#endif
