#include "GraphicsErrors.h"
#include <format>
#include <ranges>
#include <string>



void operator>>(GErrors::HrGrabber Grabber, GErrors::CheckerToken)
{
	if (FAILED(Grabber.hr)) {
		// get error description as narrow string with crlf removed
		char* pMsgBuf = nullptr;
		// windows will allocate memory for err string and make our pointer point to it
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, Grabber.hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPSTR>(&pMsgBuf), 0, nullptr
		);

		// copy error string from windows-allocated buffer to std::string
		std::string errorString = pMsgBuf;
		// free windows buffer
		LocalFree(pMsgBuf);

		errorString = errorString |
			std::ranges::views::transform([](char c) {return c == '\n' ? ' ' : c; }) |
			std::ranges::views::filter([](char c) {return c != '\r'; }) |
			std::ranges::to<std::basic_string>();
		
		printf("ERROR: %s %s %u\n",
				errorString.c_str(), Grabber.loc.file_name(), Grabber.loc.line());
		throw errorString;
	}
};

GErrors::HrGrabber::HrGrabber(unsigned int hr, std::source_location loc)  noexcept
	:
	hr(hr),
	loc(loc)
{}