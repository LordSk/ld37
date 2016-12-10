#pragma once
#include "lsk_types.h"
#include <intrin.h>
#include <thread>

typedef volatile long int vli32;

struct lsk_Mutex
{
	vli32 _inUse = 0;

	void lock() {
		while(_InterlockedExchange(&_inUse, 1) == 1) {
			// block
		}
	}

	void unlock() {
		_InterlockedExchange(&_inUse, 0);
	}
};

#ifdef _WIN32
	#include <windows.h>

	// returns HANDLE
	#define lsk_createThread(func, user_data) CreateThread(0, 0, (LPTHREAD_START_ROUTINE)func,\
		user_data, 0, nullptr)

	#define lsk_sleep(milliseconds) Sleep(milliseconds)
#endif
