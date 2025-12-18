#include "EngineTime.h"




EngineTime::EngineTime()
{
	using namespace std::chrono;

	last = steady_clock::now();
	Mark();
}
double EngineTime::Mark() noexcept
{
	using namespace std::chrono;


	//std::unique_lock<std::mutex>uLock(clockMTX);
	//auto old = frame;
	//frame = duration<double>(steady_clock::now() - last).count();
	last = steady_clock::now();
	return 0;
}
double EngineTime::Peek() noexcept
{
	using namespace std::chrono;

	//std::unique_lock<std::mutex>uLock(clockMTX);
	return duration<double>(steady_clock::now() - last).count();
}

double EngineTime::Current() noexcept
{
	using namespace std::chrono;

	//std::unique_lock<std::mutex>uLock(clockMTX);
	return frame + duration<double>(steady_clock::now() - last).count();

}

long long EngineTime::TimeLook() const
{
	using namespace std::chrono;

	return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}
