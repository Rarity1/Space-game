#include "GraphicsErrors.h"
#include <ranges>

void operator>>(GErrors::HrGrabber Grabber, GErrors::CheckerToken tkn) {
  tkn.storedfunc();
  if (FAILED(Grabber.hr)) {
    // get error description as narrow string with crlf removed
    char *pMsgBuf = nullptr;
    // windows will allocate memory for err string and make our pointer point to
    // it
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                  nullptr, Grabber.hr,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  reinterpret_cast<LPSTR>(&pMsgBuf), 0, nullptr);
    // copy error string from windows-allocated buffer to std::string
    std::string errorString(pMsgBuf);
    // free windows buffer
    LocalFree(pMsgBuf);
    errorString = errorString | std::ranges::views::transform([](char c) {
                    return c == '\n' ? ' ' : c;
                  }) |
                  std::ranges::views::filter([](char c) { return c != '\r'; }) |
                  std::ranges::to<std::basic_string>();

    printf("ERROR: %s %s %u\n", errorString.c_str(), Grabber.loc.file_name(),
           Grabber.loc.line());
    fflush(stdout);
    throw errorString;
  }else if( Grabber.hr != S_OK){
        // get error description as narrow string with crlf removed
    char *pMsgBuf = nullptr;
    // windows will allocate memory for err string and make our pointer point to
    // it
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                  nullptr, Grabber.hr,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  reinterpret_cast<LPSTR>(&pMsgBuf), 0, nullptr);
    // copy error string from windows-allocated buffer to std::string
    std::string errorString(pMsgBuf);
    // free windows buffer
    LocalFree(pMsgBuf);
    errorString = errorString | std::ranges::views::transform([](char c) {
                    return c == '\n' ? ' ' : c;
                  }) |
                  std::ranges::views::filter([](char c) { return c != '\r'; }) |
                  std::ranges::to<std::basic_string>();

    printf("INFO: %s %s %u\n", errorString.c_str(), Grabber.loc.file_name(),
           Grabber.loc.line());
    fflush(stdout);
  }
};

GErrors::HrGrabber::HrGrabber(unsigned int hr,
                              std::source_location loc) noexcept
    : hr(hr), loc(loc) {}