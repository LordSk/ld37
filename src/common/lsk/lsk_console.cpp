#include "lsk_console.h"
#include "lsk_string.h"
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

//__attribute__((format(printf, 3, 4)))
void lsk_Console::printf(u32 color, const char* format, ...)
{
	_mutex.lock();

	i32 colorOffset = 0;
	if(_color != color) {
		_color = color;
		colorOffset = 5;
		switch(color) {
			case CONSOLE_COLOR_RESET: memcpy(_line, UNIX_CONSOLE_COLOR_RESET, 4); colorOffset = 4; break;
			case CONSOLE_COLOR_RED: memcpy(_line, UNIX_CONSOLE_COLOR_RED, 5); break;
			case CONSOLE_COLOR_GREEN: memcpy(_line, UNIX_CONSOLE_COLOR_GREEN, 5); break;
			case CONSOLE_COLOR_YELLOW: memcpy(_line, UNIX_CONSOLE_COLOR_YELLOW, 5); break;
			case CONSOLE_COLOR_BLUE: memcpy(_line, UNIX_CONSOLE_COLOR_BLUE, 5); break;
			case CONSOLE_COLOR_MAGENTA: memcpy(_line, UNIX_CONSOLE_COLOR_MAGENTA, 5); break;
			case CONSOLE_COLOR_CYAN: memcpy(_line, UNIX_CONSOLE_COLOR_CYAN, 5); break;
		}
	}

	va_list args;
	va_start(args, format);
	_lineSize = vsnprintf(_line + colorOffset, MAX_LINE_SIZE - colorOffset, format, args);
	va_end(args);

	_write(STDOUT, _line, _lineSize + colorOffset);

	_mutex.unlock();
}
