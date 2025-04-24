#include "../Headers/Timer.h"

Timer::Timer(SK::DOUBLE freq) : _interval(1.0 / freq), _dt(0), _fps(0)
{
	_last = std::chrono::high_resolution_clock::now();
}

void Timer::SetFrequency(const SK::DOUBLE freq)
{
	_interval = std::chrono::duration<SK::DOUBLE>(1.0 / freq);
}

void Timer::Tick()
{
	auto now = std::chrono::high_resolution_clock::now();
	_dt = std::chrono::duration_cast<std::chrono::duration<SK::DOUBLE>>(now - _last).count();
	_fps = 1 / _dt;
	_last = now;
}

void Timer::WaitForInterval()
{
	auto now = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - _last);
	auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(_interval - elapsed);
	if (remaining.count() > 0) {
		auto c = remaining.count();
		std::this_thread::sleep_for(std::chrono::milliseconds(remaining.count()));
	}
}
