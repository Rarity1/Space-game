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
	return frameTime.count();
}

float EngineTime::Peek() const noexcept
{
	return duration<float>(high_resolution_clock::now() - last).count();
}