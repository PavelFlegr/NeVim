#pragma once
#include <vector>

enum class CursorMovement { up, down, left, right, home, end, pageup, pagedown };
struct Point {
	size_t x = 0;
	size_t y = 0;
};

bool operator<(Point p1, Point p2);
bool operator>(Point p1, Point p2);

class Editor {
public:
	Editor();
	bool write(Point pos, char text);
	void moveCursor(CursorMovement direction);
	void removeLast();
	bool remove(Point pos);
	void removeRange(Point start, Point end);
	void removeNext();
	bool linebreak(Point pos);
	Point getCursorPos();
	std::vector<std::vector<char>> buffer;

private:
	bool isValidPos(Point pos);
	Point cursor;
};