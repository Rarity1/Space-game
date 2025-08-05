#include "EngineTime.h"
using namespace std::chrono;




EngineTime::EngineTime()
{
	last = high_resolution_clock::now();
}
double EngineTime::Mark() noexcept
{
	std::unique_lock<std::mutex>uLock(clockMTX);
	const auto old = last;
	last = high_resolution_clock::now();
	frame = duration<double>(last - old).count();
	return frame;
}
double EngineTime::Peek() noexcept
{
	std::unique_lock<std::mutex>uLock(clockMTX);
	auto result = duration<double>(high_resolution_clock::now() - last).count();
	return result;
}

double EngineTime::Current() noexcept
{
	std::unique_lock<std::mutex>uLock(clockMTX);
	auto result = frame + duration<double>(high_resolution_clock::now() - last).count();
	return result;
}

long long EngineTime::TimeLook() const
{
	long long result = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();

	return result;
}
