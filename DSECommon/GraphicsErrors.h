#pragma once
#include <source_location>
#include <ranges>
#include <format>
#include <wrl.h>
#include "CWin.h"

class DLL GErrors {
public:
	struct CheckerToken {
		CheckerToken() = default;
		bool operator==(const CheckerToken& other) const = default;
	};
	CheckerToken chk;
	struct HrGrabber {
		HrGrabber(unsigned int hr, std::source_location = std::source_location::current()) noexcept;
		unsigned int hr;
		std::source_location loc;
	};
	
};

void operator >>(GErrors::HrGrabber, GErrors::CheckerToken);