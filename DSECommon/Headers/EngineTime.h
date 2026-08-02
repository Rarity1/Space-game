#pragma once
#include <chrono>
#include <mutex>

class EngineTime
{
public:
	EngineTime();
	double Mark() noexcept;
	double Peek()noexcept;
	double Current() noexcept;
	long long TimeLook() const;
	std::chrono::utc_clock::time_point last;

private:
	std::mutex clockMTX;
	double frame = 0.0;
};