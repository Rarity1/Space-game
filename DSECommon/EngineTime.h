#pragma once
#include "CWin.h"
#include <chrono>

class DLL EngineTime
{
public:
	EngineTime();
	double Mark() noexcept;
	double Peek() const noexcept;
	double Current() const;
	long long TimeLook() const;
private:
	std::chrono::high_resolution_clock::time_point last;
	double frame = 0.0;
};