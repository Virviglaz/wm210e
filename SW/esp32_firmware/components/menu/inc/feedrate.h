#ifndef __FEEDRATE_H__
#define __FEEDRATE_H__

#include <stdint.h>
#include <vector>
#include "motor_ctrl.h"
#include "menu.h"

class FeedRateType {
public:
	const char *title;
	uint32_t step;
};

static const std::vector<FeedRateType> thread_list {
	{ .title = "M2x0.4",	.step = 50 },
	{ .title = "M3x0.5",	.step = 40 },
	{ .title = "M4x0.7",	.step = 57 / 2 },
	{ .title = "M5x0.8",	.step = 25 },
	{ .title = "M6x1.0",	.step = 20 },
	{ .title = "M8x1.25",	.step = 16 },
	{ .title = "M10x1.5",	.step = 27 / 2 },
	{ .title = "M12x1.75",	.step = 23 / 2 },
	{ .title = "M14x2.0",	.step = 10 },
	{ .title = "M16x2.0",	.step = 10 }
};

static const std::vector<FeedRateType> feedrate_list {
	{ .title = "0.05 mm/r",	.step = 400 },
	{ .title = "0.10 mm/r",	.step = 200 },
	{ .title = "0.25 mm/r",	.step = 80  },
	{ .title = "0.50 mm/r",	.step = 40  }
};

class FeedRateMenu : public MenuItem
{
	Child<FeedRateType> list;
	FeedRateType current;
	dir direction;
public:
	FeedRateMenu() { }

	FeedRateMenu(std::string title,
		   enum dir dir,
		   Child<FeedRateType> step_list) {
		title_str = title;
		direction = dir;
		list = step_list;
		current = list.get_first();
	}

	void next() {
		current = list.next();
	}

	void prev() {
		current = list.prev();
	}

	MenuItem *enter(lcd& lcd, Buttons& btns) {
		auto item = list.get_current();
		thread_cut(lcd, btns, item.title, item.step, direction == CW);
		return MenuItem::back();
	}

	MenuItem *back() {
		return MenuItem::back();
	}

	void update_lcd(lcd& lcd) {
		auto item = list.get_current();
		lcd.clear();
		lcd.print(FIRST_ROW,  CENTER, "%s", title_str.c_str());
		lcd.print(SECOND_ROW, CENTER, "%s", item.title);
	}
};

#endif /* __FEEDRATE_H__ */
