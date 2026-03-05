#include "GraphicsErrors.h"



void operator>>(GErrors::HrGrabber g, GErrors::CheckerToken)
{
	if (FAILED(g.hr)) {
		// get error description as narrow string with crlf removed
		char* pMsgBuf = nullptr;
		// windows will allocate memory for err string and make our pointer point to it
		const DWORD nMsgLen = FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, g.hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
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
		
		throw std::runtime_error{
			std::format("Error: {}\n   {}({})",
				errorString, g.loc.file_name(), g.loc.line())
		};
	}
};

GErrors::HrGrabber::HrGrabber(unsigned int hr, std::source_location loc)  noexcept
	:
	hr(hr),
	loc(loc)
{}