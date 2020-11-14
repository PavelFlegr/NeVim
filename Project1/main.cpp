#define NOMINMAX
#define _WIN32_WINNT 0x0500
#include <iostream>
#include <Windows.h>
#include <vector>
#include <stdlib.h>
#include "Editor.h"
#include <algorithm>
#define MAXEVENTS 100



HANDLE cin = GetStdHandle(STD_INPUT_HANDLE);
HANDLE cout = GetStdHandle(STD_OUTPUT_HANDLE);
INPUT_RECORD input[MAXEVENTS];
Point cursor;
Point scroll;
void redrawConsole();
COORD editorSize;
Point selectionAnchor;
bool selectionActive;
CHAR_INFO* buffer;

HWND hwnd;

void autoScroll();
short tabSize = 4;

Editor editor;
short startColumn = 0;
void handleKeyEvent(KEY_EVENT_RECORD event);
void dispatchEvents(int eventcount);

COORD getEditorSize();
int readConsoleInput();
size_t getDisplayXPos();

int main() {
	editor = Editor();
	DWORD mode;
	hwnd = GetConsoleWindow();
	GetConsoleMode(cin, &mode);
	SetConsoleMode(cin, mode & !ENABLE_PROCESSED_INPUT);
	cursor = { 0,0 };
	while (true) {
		editorSize = getEditorSize();
		int eventcount = readConsoleInput();
		dispatchEvents(eventcount);
		autoScroll();
		redrawConsole();
	}
}

void startSelection() {
	selectionAnchor = {cursor.x, cursor.y};
	selectionActive = true;
}

void stopSelection() {
	selectionActive = false;
}

//reads events from console buffer into input buffer and returns number of events read
int readConsoleInput() {
	DWORD eventcount;
	ReadConsoleInput(cin, input, MAXEVENTS, &eventcount);

	return eventcount;
}

void dispatchEvents(int eventcount) {
	for (int i = 0; i < eventcount; i++) {
		switch (input[i].EventType) {
		case KEY_EVENT:
			handleKeyEvent(input[i].Event.KeyEvent);
		}
	}
}


bool operator < (COORD c1, COORD c2) {
	if (c1.Y < c2.Y || (c1.Y == c2.Y && c1.X < c2.X)) {
		return true;
	}
	else {
		return false;
	}

}

bool operator > (COORD c1, COORD c2) {
	if (c1.Y > c2.Y || (c1.Y == c2.Y && c1.X > c2.X)) {
		return true;
	}
	else {
		return false;
	}

}

void deleteSelection() {
	if (selectionActive) {
		Point start = std::min(selectionAnchor, cursor);
		Point end = std::max(selectionAnchor, cursor);
		editor.removeRange(start, end);
		cursor = start;
		stopSelection();
	}
}

void autoScroll() {
	if (getDisplayXPos() - scroll.x + 1 > (size_t)editorSize.X) {
		scroll.x = getDisplayXPos() - editorSize.X + 1;
	}

	if (getDisplayXPos() < scroll.x) {
		scroll.x = getDisplayXPos();
	}

	if (cursor.y - scroll.y + 1 >= (size_t)editorSize.Y) {
		scroll.y = cursor.y - editorSize.Y + 1;
	}

	if (cursor.y < scroll.y) {
		scroll.y = cursor.y;
	}
}

void moveCursor(CursorMovement direction, bool shiftPressed) {
	Point max = { editor.buffer[cursor.y].size(), editor.buffer.size() - 1 };

	bool changeX = false;
	if (shiftPressed && !selectionActive) {
		startSelection();
	}
	if (!shiftPressed && selectionActive) {
		stopSelection();
	}
	switch (direction) {
	case CursorMovement::right:
		//move the cursor to the right if possible, otherwise fall through to move down
		if (cursor.x != max.x) {
			cursor = { cursor.x + 1, cursor.y };
			
			break;
		}
		else {
			changeX = true;
		}
	case CursorMovement::down:
		if (cursor.y != max.y) {
			cursor = { cursor.x, cursor.y + 1 };
			max.x = editor.buffer[cursor.y].size();
			if (changeX) {
				cursor = { 0, cursor.y };
			}
			if (cursor.x >= max.x) {
				cursor = { max.x, cursor.y };
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
			max.x = editor.buffer[cursor.y].size();
			if (changeX) {
				cursor = { max.x, cursor.y };
			}
			if (cursor.x >= max.x) {
				cursor = { max.x, cursor.y };
			}
		}
		break;
	case CursorMovement::home:
		cursor.x = 0;
		break;
	case CursorMovement::end:
		cursor.x = editor.buffer[cursor.y].size();
		break;
	case CursorMovement::pagedown:
	{
		cursor.y += editorSize.Y - 1;
		scroll.y += editorSize.Y - 1;
		size_t maxScroll = editor.buffer.size() > editorSize.Y ? editor.buffer.size() - editorSize.Y : 0;
		if (maxScroll < scroll.y) {
			scroll.y = maxScroll;
			cursor.y = editor.buffer.size() - 1;
		}
		cursor.x = std::min(cursor.x, editor.buffer[cursor.y].size());
		break;
	}
	case CursorMovement::pageup:
		cursor.y = cursor.y > editorSize.Y - 1 ? cursor.y - (editorSize.Y - 1) : 0;
		scroll.y = scroll.y > editorSize.Y - 1 ? scroll.y - (editorSize.Y - 1) : 0;
		cursor.x = std::min(cursor.x, editor.buffer[cursor.y].size());
		break;
	}
}

int getSelectionLength() {
	
	Point start = std::min(selectionAnchor, cursor);
	Point end = std::max(selectionAnchor, cursor);
	
	if (start.y == end.y) {
		return end.x - start.x + 1;
	}

	int length = 0;


	length += editor.buffer[start.y].size() - start.x + 2; //2 extra for cr-lf
	length += end.x;
	for (size_t i = start.y + 1; i < std::max(cursor.y, selectionAnchor.y); i++) {
		length += editor.buffer[i].size() + 2; //2 extra for cr-lf
	}

	return length + 1; //1 extra for null;
}

void copySelection() {
	char crlf[] = "\r\n";
	if (!selectionActive) {
		return;
	}
	int length = getSelectionLength();
	HANDLE objHandle = GlobalAlloc(GMEM_MOVEABLE, length * sizeof(char));
	char* buffer = (char*)GlobalLock(objHandle);
	
	Point start = std::min(selectionAnchor, cursor);
	Point end = std::max(selectionAnchor, cursor);
	int pos = 0;

	if (start.y == end.y) {
		std::copy(editor.buffer[start.y].begin() + start.x, editor.buffer[start.y].begin() + end.x, buffer);
	}
	else {
		std::copy(editor.buffer[start.y].begin() + start.x, editor.buffer[start.y].end(), buffer);
		pos += editor.buffer[start.y].size() - start.x;
		std::copy(crlf, crlf + 2, buffer + pos);
		pos += 2;
		for (size_t i = start.y + 1; i < std::max(cursor.y, selectionAnchor.y); i++) {
			std::copy(editor.buffer[i].begin(), editor.buffer[i].end(), buffer + pos);
			pos += editor.buffer[i].end() - editor.buffer[i].begin();
			std::copy(crlf, crlf + 2, buffer + pos);
			pos += 2;
		}
		std::copy(editor.buffer[end.y].begin(), editor.buffer[end.y].begin() + end.x, buffer + pos);
	}
	buffer[length - 1] = 0;
	GlobalUnlock(objHandle);
	OpenClipboard(hwnd);
	EmptyClipboard();
	if (SetClipboardData(CF_TEXT, objHandle) == NULL) {
		DWORD er = GetLastError();
	}
	
	CloseClipboard();
}

void pasteClipboard() {
	OpenClipboard(hwnd);
	HANDLE objHandle = GetClipboardData(CF_TEXT);
	if (objHandle == NULL) {
		return;
	}
	char* buffer = (char*)GlobalLock(objHandle);
	while (*buffer != 0) {
		switch (*buffer) {
			//ignore \r because it should always be followed by \n while \n could also be standalone
		case '\r':
			break;
		case '\n':
			editor.linebreak({ cursor.x, cursor.y });
			cursor.y++;
			cursor.x = 0;
			break;
		default:
			editor.write({ cursor.x, cursor.y }, *buffer);
			cursor.x++;
		}
		buffer++;
	}
	GlobalUnlock(objHandle);
	CloseClipboard();
}

void handleKeyEvent(KEY_EVENT_RECORD event) {
	if (event.bKeyDown) {
		//control keys
		if ((event.dwControlKeyState & LEFT_CTRL_PRESSED) || (event.dwControlKeyState & RIGHT_CTRL_PRESSED)) {
			if (event.uChar.AsciiChar != 0) {
				int t = 1;
			}
			switch (event.uChar.AsciiChar) {
			//ctrl-c
			case 3:
				copySelection();
				break;
			//ctrl-x
			case 24:
				copySelection();
				deleteSelection();
				break;
			//ctrl-v
			case 22:
				deleteSelection();
				pasteClipboard();
				break;
			}
		}
		//normal keys
		else {
			switch (event.wVirtualKeyCode) {
			case VK_LEFT:
				if (event.dwControlKeyState & SHIFT_PRESSED && !selectionActive) {
					startSelection();
				}
				moveCursor(CursorMovement::left, event.dwControlKeyState & SHIFT_PRESSED);


				break;
			case VK_RIGHT:
				if (event.dwControlKeyState & SHIFT_PRESSED && !selectionActive) {
					startSelection();
				}
				moveCursor(CursorMovement::right, event.dwControlKeyState & SHIFT_PRESSED);
				break;
			case VK_UP:
				moveCursor(CursorMovement::up, event.dwControlKeyState & SHIFT_PRESSED);
				break;
			case VK_DOWN:
				moveCursor(CursorMovement::down, event.dwControlKeyState & SHIFT_PRESSED);
				break;
			case VK_RETURN:
				deleteSelection();
				if (editor.linebreak({ cursor.x, cursor.y })) {
					cursor.x = 0;
					cursor.y++;
				}

				break;
			case VK_BACK:
				if (selectionActive) {
					deleteSelection();
				}
				else {
					size_t length;
					if (cursor.x == 0 && cursor.y < editor.buffer.size() && cursor.y != 0) {
						length = editor.buffer[cursor.y - 1].size();
					}
					if (editor.remove({ cursor.x - 1, cursor.y })) {
						if (cursor.x == 0) {
							cursor.x = length;
							cursor.y--;
						}
						else {
							cursor.x--;
						}
					}
				}
				break;
			case VK_DELETE:
				if (selectionActive) {
					deleteSelection();
				}
				else {
					editor.remove({ cursor.x, cursor.y });
				}
				break;
			case VK_HOME:
				moveCursor(CursorMovement::home, event.dwControlKeyState & SHIFT_PRESSED);
				break;
			case VK_END:
				moveCursor(CursorMovement::end, event.dwControlKeyState & SHIFT_PRESSED);
				break;
			case VK_NEXT:
				moveCursor(CursorMovement::pagedown, event.dwControlKeyState & SHIFT_PRESSED);
				break;
			case VK_PRIOR:
				moveCursor(CursorMovement::pageup, event.dwControlKeyState & SHIFT_PRESSED);
			default:
				if (event.uChar.AsciiChar != 0) {
					if (editor.write({ cursor.x, cursor.y }, event.uChar.AsciiChar)) {
						moveCursor(CursorMovement::right, false);
					}
				}
			}
		}
	}
}

COORD getEditorSize() {
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo(cout, &info);
	return { info.srWindow.Right, info.srWindow.Bottom };
}

size_t getDisplayXPos() {
	int x = 0;
	for (size_t i = 0; i < cursor.x; i++) {
		if (editor.buffer[cursor.y][i] == '\t') {
			x += tabSize;
		}
		else {
			x++;
		}
	}

	return x;
}

bool isWithin(Point needle, Point bound1, Point bound2) {
	Point start = std::min(bound1, bound2);
	Point end = std::max(bound1, bound2);
	if (bound1.y < bound2.y || (bound1.y == bound2.y && bound1.x < bound2.x)) {
		start = bound1;
		end = bound2;
	}
	else {
		start = bound2;
		end = bound1;
	}
	if (needle.y < end.y && needle.y > start.y) {
		return true;
	}
	else if (needle.y == end.y && needle.y == start.y && (needle.x < start.x || needle.x >= end.x)) {
		return false;
	}
	else if (needle.y == end.y && needle.x < end.x) {
		return true;
	}
	else if (needle.y == start.y && needle.x >= start.x) {
		return true;
	}

	return false;
}

CHAR_INFO* prepareScreen() {
	auto buffer = new CHAR_INFO[editorSize.X * editorSize.X];
	CHAR_INFO ci;
	short rowOffset = 0;
	for (size_t row = scroll.y; row < editorSize.Y + scroll.y; row++) {
		size_t displayColumn = 0;
		size_t column = 0;
		int tabCount = 0;
		while (displayColumn < editorSize.X + scroll.x) {
			if (selectionActive && isWithin({ column, row }, selectionAnchor, cursor) && column < editor.buffer[row].size()) {
				ci.Attributes = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED;
			}
			else {
				ci.Attributes = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
			}
			if (row >= editor.buffer.size() || column >= editor.buffer[row].size()) {
				ci.Char.AsciiChar = ' ';
			}
			else {
				char c = editor.buffer[row][column];
				if (c == '\t') {
					if (tabCount == 0) {
						tabCount = tabSize;
						ci.Char.AsciiChar = ' ';
					}
					if (tabCount == 1) {
						tabCount = 0;
						column++;
					}
					else {
						tabCount--;
					}
				}
				else {
					ci.Char.AsciiChar = c;
					column++;
				}
			}
			if (displayColumn >= scroll.x) {
				buffer[rowOffset + displayColumn - scroll.x] = ci;
			}
			displayColumn++;
		}
		rowOffset += editorSize.X;
	}

	return buffer;
}

void redrawConsole() {
	CHAR_INFO* buffer = prepareScreen();
	SMALL_RECT r;
	r.Top = 0;
	r.Left = 0;
	r.Right = editorSize.X;
	r.Bottom = editorSize.Y;
	WriteConsoleOutput(cout, buffer, {editorSize.X, editorSize.Y}, { 0,0 }, &r);
	SetConsoleCursorPosition(cout, { (SHORT)(getDisplayXPos() - scroll.x), (SHORT)(cursor.y - scroll.y)});
	delete[] buffer;
}