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

	f64 _time = 0;
	lsk_Array<Timer, 1024> _timers;

	void reset() {
		_timers.clear();
		_time = 0;
	}

	void udpate(f64 delta) {
		_time += delta;
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

	inline f64 getTime() const {
		return _time;
	}
};

#define Timers TimerManager::get()
