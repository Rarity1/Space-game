#include "Profiler.h"
#include <source_location>
#include <string>
#include "Exceptions.h"


Profiler::Profiler(std::source_location loc, std::chrono::utc_clock::time_point time):here(loc), TimeKeeper(time){}
Profiler::~Profiler(){
    tsPrintBuffer::QueuePrintF(
        "Time Taken: {}(Seconds) \n Location: {} \n Line: {} \n",
        std::to_string(std::chrono::duration<double>(std::chrono::utc_clock::now() - TimeKeeper).count()), here.function_name(), here.line());
}

void Profiler::Check(){
    tsPrintBuffer::QueuePrintF(
        "Time Taken: {}(Seconds) \n Location: {} \n Line: {} \n",
        std::to_string(std::chrono::duration<double>(std::chrono::utc_clock::now() - TimeKeeper).count()), here.function_name(), here.line());
}