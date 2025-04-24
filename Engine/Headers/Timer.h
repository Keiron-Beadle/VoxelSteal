#pragma once
#include <chrono>
#include "gil.h"
#include <cstdint>

class Timer {
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> _last;
	std::chrono::duration<SK::DOUBLE> _interval;
	SK::DOUBLE _dt;
	SK::DOUBLE _fps;

public:
	Timer(SK::DOUBLE freq = 60);
	~Timer() = default;

	nodconst SK::DOUBLE dT() const { return _dt; }
	nodconst SK::DOUBLE FPS() const { return _fps; }

	void SetFrequency(const SK::DOUBLE freq);

	void Tick();

	void WaitForInterval();
};