#pragma once
#include <chrono>
#include <mutex>

class EngineTime
{
public:
	EngineTime();
	double Mark() noexcept;
	double Peek()const noexcept;
	double Current() const noexcept;
	long long TimeLook() const noexcept;
	std::chrono::utc_clock::time_point last;

private:
	std::mutex clockMTX;
	double frame = 0.0;
};