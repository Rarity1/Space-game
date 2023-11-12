#include "EngineTime.h"
using namespace std::chrono;

EngineTime::EngineTime() noexcept
{
	last = high_resolution_clock::now();
}
float EngineTime::Mark() noexcept
{
	const auto old = last;
	last = high_resolution_clock::now();
	const duration<float> frameTime = last - old;
	std::chrono::milliseconds d = std::chrono::duration_cast<std::chrono::milliseconds>(frameTime);
	return d.count();
}

float EngineTime::Peek() const noexcept
{
	duration<float> fs(high_resolution_clock::now() - last);
	std::chrono::milliseconds d = std::chrono::duration_cast<std::chrono::milliseconds>(fs);
	return d.count();
}