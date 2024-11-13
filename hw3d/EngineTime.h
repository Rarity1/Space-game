#pragma once
#include <chrono>

class EngineTime
{
public:
	EngineTime();
	double Mark() noexcept;
	double Peek() const noexcept;
	double Current() const;
private:
	std::chrono::high_resolution_clock::time_point last;
	double frame = 0.0;
};