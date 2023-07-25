#ifndef __MENU_H__
#define __MENU_H__

#include <vector>
#include <string>
#include "lcd.h"

void menu_start(const char *version);

template <typename T> class Child
{
	std::vector<T> list;
	int n = 0;
public:
	Child() {}

	Child(std::vector<T> child_list) : list(child_list) {}

	T next() {
		n = n == list.capacity() - 1 ? 0 : n + 1;
		return list[n];
	}

	T prev() {
		n = n == 0 ? list.capacity() - 1 : n - 1;
		return list[n];
	}

	T get_current() {
		return list[n];
	}
};

class MenuItem
{
protected:
	std::string title_str;
	Child<MenuItem *> list;
	MenuItem *parent = nullptr;
	MenuItem *current;
public:
	MenuItem() {}

	MenuItem(std::string title) {
		title_str = title;
	}

	MenuItem(std::string title, std::vector<MenuItem *> children) {
		title_str = title;
		list = Child(children);
		current = children[0];
	}

	virtual void next() {
		current = list.next();
	}

	virtual void prev() {
		current = list.prev();
	}

	virtual MenuItem *enter() {
		parent = this;
		return current;
	}

	virtual MenuItem *back() {
		return parent ? parent : this;
	}

	virtual void update_lcd(lcd& lcd) {
		lcd.clear();
		auto item = list.get_current();
		lcd.print(FIRST_ROW,  CENTER, "%s", title_str.c_str());
		lcd.print(SECOND_ROW, CENTER, "%s", item->title_str.c_str());
	}
};

#endif /* __MENU_H__ */
