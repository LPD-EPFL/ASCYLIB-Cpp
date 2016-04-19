
#ifndef ___SEARCH_H_
#define ___SEARCH_H_

template<typename KEY_TYPE, typename VALUE_TYPE>
class Search
{
	public:
		virtual VALUE_TYPE search(KEY_TYPE key) = 0;
		virtual int insert(KEY_TYPE key, VALUE_TYPE val) = 0;
		virtual VALUE_TYPE remove(KEY_TYPE key) = 0;
		virtual int length() = 0;
};

#endif
