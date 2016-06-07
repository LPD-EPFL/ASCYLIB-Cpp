#ifndef _STACK_QUEUE_H_
#define _STACK_QUEUE_H_

template <typename T>
class StackQueue
{
	public:
		virtual int add(T item) = 0;
		virtual T remove() = 0;
		virtual int count() = 0;
};

#endif
