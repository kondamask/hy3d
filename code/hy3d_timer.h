#pragma once
#include <chrono>

using time_point = std::chrono::steady_clock::time_point;

class Timer
{
public:
	Timer();
	float Mark();
	float Peek() const;
private:
	time_point last;
};

