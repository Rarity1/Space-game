#include <chrono>
#include <source_location>

class Profiler{
  public:
  Profiler(std::source_location = std::source_location::current(), std::chrono::utc_clock::time_point = std::chrono::utc_clock::now() );
  void Check();
  ~Profiler();
  private:
  const std::source_location here;
  const std::chrono::utc_clock::time_point TimeKeeper;

};