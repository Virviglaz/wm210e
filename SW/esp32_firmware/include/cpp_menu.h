#ifndef __CPP_MENU_H__
#define __CPP_MENU_H__

#include <string>
#include <vector>
#include <atomic>

template <class T>
class Menu
{
public:
	Menu(const std::vector<T>& _list) : list(_list) {}

	void next() {
		i++;
		if (i == list.size())
			i = 0;
	}

	void prev() {
		if (i)
			i--;
		else
			i = list.size() - 1;
	}

	const T *get() {
		return &list.at(i);
	}
private:
	std::atomic<int> i { 0 };
	const std::vector<T>& list;
};

#endif /* __CPP_MENU_H__ */
