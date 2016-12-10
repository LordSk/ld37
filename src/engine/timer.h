#pragma once
#include <functional>
#include <lsk/lsk_array.h>

typedef std::function<void()> TimerFunction;

struct TimerManager
{
	SINGLETON_IMP(TimerManager)

	struct Timer {
		TimerFunction f;
		f64 delay;
	};

	lsk_Array<Timer, 1024> _timers;

	void udpate(f64 delta) {
		for(i32 i = 0; i < _timers.count(); ++i) {
			_timers[i].delay -= delta;
			if(_timers[i].delay <= 0.0) {
				_timers[i].f();
				_timers.remove(_timers[i]);
				--i;
			}
		}
	}

	inline void add(f64 delay, TimerFunction f) {
		_timers.push({f, delay});
	}
};

#define Timers TimerManager::get()
