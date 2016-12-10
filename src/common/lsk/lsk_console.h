#pragma once
#include "lsk_types.h"
#include "lsk_thread.h"

#define UNIX_CONSOLE_COLOR_RED     "\x1b[31m"
#define UNIX_CONSOLE_COLOR_GREEN   "\x1b[32m"
#define UNIX_CONSOLE_COLOR_YELLOW  "\x1b[33m"
#define UNIX_CONSOLE_COLOR_BLUE    "\x1b[34m"
#define UNIX_CONSOLE_COLOR_MAGENTA "\x1b[35m"
#define UNIX_CONSOLE_COLOR_CYAN    "\x1b[36m"
#define UNIX_CONSOLE_COLOR_RESET   "\x1b[0m"

enum: u32 {
	CONSOLE_COLOR_RESET=0,
	CONSOLE_COLOR_RED,
	CONSOLE_COLOR_GREEN,
	CONSOLE_COLOR_YELLOW,
	CONSOLE_COLOR_BLUE,
	CONSOLE_COLOR_MAGENTA,
	CONSOLE_COLOR_CYAN,
};

struct lsk_Console
{
	SINGLETON_IMP(lsk_Console)

	enum: u32 {
		STDIN=0,
		STDOUT=1,
		STDERR=2,

		MAX_LINE_SIZE=4096,
	};

	char _line[MAX_LINE_SIZE];
	i32 _lineSize = 0;
	u32 _color = CONSOLE_COLOR_RESET;
	lsk_Mutex _mutex;

	void printf(u32 color, const char* format, ...);
};

#define lsk_printf(format, ...) lsk_Console::get().printf(CONSOLE_COLOR_RESET, format "\n", ##__VA_ARGS__)
#define lsk_warnf(format, ...) lsk_Console::get().printf(CONSOLE_COLOR_YELLOW, format "\n", ##__VA_ARGS__)
#define lsk_errf(format, ...) lsk_Console::get().printf(CONSOLE_COLOR_RED, format "\n", ##__VA_ARGS__)
#define lsk_succf(format, ...) lsk_Console::get().printf(CONSOLE_COLOR_GREEN, format "\n", ##__VA_ARGS__)

#ifndef NDEBUG
	#define lsk_debugf(format, ...) lsk_Console::get().printf(CONSOLE_COLOR_YELLOW, format "\n"\
	, ##__VA_ARGS__)
#else
	#define lsk_debugf(format, ...)
#endif

