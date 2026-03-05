#include "EngineTime.h"




EngineTime::EngineTime()
{
	using namespace std::chrono;

	last = utc_clock::now();
	Mark();
}
double EngineTime::Mark() noexcept
{
	using namespace std::chrono;


	//std::unique_lock<std::mutex>uLock(clockMTX);
	//auto old = frame;
	frame = duration<double>(utc_clock::now() - last).count();
	last = utc_clock::now();
	return frame;
}
double EngineTime::Peek() noexcept
{
	using namespace std::chrono;

	//std::unique_lock<std::mutex>uLock(clockMTX);
	return duration<double>(utc_clock::now() - last).count();
}

double EngineTime::Current() noexcept
{
	using namespace std::chrono;

	//std::unique_lock<std::mutex>uLock(clockMTX);
	return frame + duration<double>(utc_clock::now() - last).count();

}

long long EngineTime::TimeLook() const
{
	using namespace std::chrono;

	return duration_cast<nanoseconds>(utc_clock::now().time_since_epoch()).count();
}
