#include "Editor.h"
#include <stdlib.h>
#include <algorithm>


Editor::Editor() {
	buffer.push_back(std::vector<char>());
}

bool Editor::write(Point pos, char text) {
	if (isValidPos(pos)) {
		buffer[pos.y].insert(buffer[pos.y].begin() + pos.x, text);
		return true;
	}
	return false;
	moveCursor(CursorMovement::right);
}

void Editor::moveCursor(CursorMovement direction) {
	size_t maxX = buffer[cursor.y].size();
	size_t maxY = buffer.size() - 1;
	bool changeX = false;
	switch (direction) {
	case CursorMovement::right:
		//move the cursor to the right if possible, otherwise fall through to move down
		if (cursor.x != maxX) {
			cursor = { cursor.x + 1, cursor.y };
			break;
		}
		else {
			changeX = true;
		}
	case CursorMovement::down:
		if (cursor.y != maxY) {
			cursor = { cursor.x, cursor.y + 1 };
			maxX = buffer[cursor.y].size();
			if (changeX) {
				cursor = { 0, cursor.y };
			}
			if (cursor.x >= maxX) {
				cursor = { maxX, cursor.y };
			}
		}
		break;
	case CursorMovement::left:
		if (cursor.x != 0) {
			cursor = { cursor.x - 1, cursor.y };
			break;
		}
		else {
			changeX = true;
		}
	case CursorMovement::up:
		if (cursor.y != 0) {
			cursor = { cursor.x,cursor.y - 1 };
			maxX = buffer[cursor.y].size();
			if (changeX) {
				cursor = { maxX, cursor.y };
			}
			if (cursor.x >= maxX) {
				cursor = { maxX, cursor.y };
			}
		}
		break;
	}
}

void Editor::removeRange(Point bound1, Point bound2) {
	Point start = std::min(bound1, bound2);
	Point end = std::max(bound1, bound2);
	//if we are deleting only the content of 1 line
	if (start.y == end.y) {
		buffer[start.y].erase(buffer[start.y].begin() + start.x, buffer[start.y].begin() + end.x);

		return;
	}

	//otherwise we are deleting a block of lines

	//first delete the characters on the first line
	buffer[start.y].erase(buffer[start.y].begin() + start.x, buffer[start.y].end());

	//next move the end of last line to the end of the first one
	buffer[start.y].insert(buffer[start.y].end(), buffer[end.y].begin() + end.x, buffer[end.y].end());

	//and lastly remove any lines in between the selection
	for (size_t i = 0; i < end.y - start.y; i++) {
		buffer.erase(buffer.begin() + start.y + 1);
	}
}

bool Editor::remove(Point pos) {
	if (pos.y < buffer.size()) {
		if (pos.x < buffer[pos.y].size()) {
			buffer[pos.y].erase(buffer[pos.y].begin() + pos.x);
			return true;
		}

		if (pos.y != 0 && pos.x == -1) {
			buffer[pos.y - 1].insert(buffer[pos.y - 1].end(), buffer[pos.y].begin(), buffer[pos.y].end());
			buffer.erase(buffer.begin() + pos.y);

			return true;
		}

		if (pos.y + 1 < buffer.size() && pos.x == buffer[pos.y].size()) {
			buffer[pos.y].insert(buffer[pos.y].end(), buffer[pos.y + 1].begin(), buffer[pos.y + 1].end());
			buffer.erase(buffer.begin() + pos.y + 1);
			return true;
		}
	}

	return false;
}

void Editor::removeLast() {
	if (cursor.x > 0) {
		buffer[cursor.y].erase(buffer[cursor.y].begin() + cursor.x - 1);
		moveCursor(CursorMovement::left);
	}
	else if (cursor.y > 0) {
		cursor.y -= 1;
		cursor.x = buffer[cursor.y].size();
		buffer[cursor.y].insert(buffer[cursor.y].end(), buffer[cursor.y + 1].begin(), buffer[cursor.y + 1].end());
		buffer.erase(buffer.begin() + cursor.y + 1);
	}
}

void Editor::removeNext() {
	if (cursor.x < buffer[cursor.y].size()) {
		buffer[cursor.y].erase(buffer[cursor.y].begin() + cursor.x);
	}
	else if (cursor.y < buffer.size() - 1) {
		cursor.x = buffer[cursor.y].size();
		buffer[cursor.y].insert(buffer[cursor.y].end(), buffer[cursor.y + 1].begin(), buffer[cursor.y + 1].end());
		buffer.erase(buffer.begin() + cursor.y + 1);
	}
}

Point Editor::getCursorPos() {
	return cursor;
}

bool Editor::linebreak(Point pos) {
	if (pos.y < buffer.size() && pos.x <= buffer[pos.y].size() + 1) {
		buffer.insert(buffer.begin() + pos.y, std::vector<char>());
		buffer[pos.y] = std::vector<char>(buffer[pos.y + 1].begin(), buffer[pos.y + 1].begin() + pos.x);
		buffer[pos.y + 1].erase(buffer[pos.y + 1].begin(), buffer[pos.y + 1].begin() + pos.x);
		
		return true;
	}

	return false;
}

bool Editor::isValidPos(Point pos) {
	return pos.y < buffer.size() && pos.x <= buffer[pos.y].size();
}

bool operator < (Point p1, Point p2) {
	if (p1.y < p2.y || (p1.y == p2.y && p1.x < p2.x)) {
		return true;
	}
	else {
		return false;
	}

}

bool operator > (Point p1, Point p2) {
	if (p1.y > p2.y || (p1.y == p2.y && p1.x > p2.x)) {
		return true;
	}
	else {
		return false;
	}

}