#include "EngineTime.h"
using namespace std::chrono;




EngineTime::EngineTime()
{
	last = high_resolution_clock::now();
}
double EngineTime::Mark() noexcept
{
	const auto old = last;
	last = high_resolution_clock::now();
	const duration<double> frameTime = last - old;
	frame = frameTime.count();
	return frame;
}
double EngineTime::Peek() const noexcept
{
	duration<double> fs(high_resolution_clock::now() - last);
	return fs.count();
}

double EngineTime::Current() const
{
	return frame;
}