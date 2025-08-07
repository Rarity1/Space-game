#include "EngineTime.h"
using namespace std::chrono;




EngineTime::EngineTime()
{
	last = high_resolution_clock::now();
}
double EngineTime::Mark() noexcept
{
	//std::unique_lock<std::mutex>uLock(clockMTX);
	frame = duration<double>(high_resolution_clock::now() - last).count();
	last = high_resolution_clock::now();
	return frame;
}
double EngineTime::Peek() noexcept
{
	//std::unique_lock<std::mutex>uLock(clockMTX);
	return duration<double>(high_resolution_clock::now() - last).count();
}

double EngineTime::Current() noexcept
{
	//std::unique_lock<std::mutex>uLock(clockMTX);
	return frame + duration<double>(high_resolution_clock::now() - last).count();
}

long long EngineTime::TimeLook() const
{

	return duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
}
