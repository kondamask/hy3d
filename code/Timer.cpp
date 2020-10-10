#include "Timer.h"

Timer::Timer()
	: last(std::chrono::steady_clock::now())
{
}

float Timer::Mark()
{
	const time_point old = last;
	last = std::chrono::steady_clock::now();
	const std::chrono::duration<float> elapsed = last - old;
	return elapsed.count();
}

float Timer::Peek() const
{
	return std::chrono::duration<float>(std::chrono::steady_clock::now() - last).count();
}
