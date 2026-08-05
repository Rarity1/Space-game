#include "GraphicsErrors.h"
#include <ranges>

void operator>>(GErrors::ErrGrab Grabber, GErrors::CheckerToken tkn) {
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
  } else if (Grabber.hr != S_OK) {
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

GErrors::ErrGrab::ErrGrab(unsigned int hr,
                              std::source_location loc) noexcept
    : hr(hr), loc(loc) {}



std::string
GErrors::D3D12MessageCategoryToString(D3D12_MESSAGE_CATEGORY Category) {
  switch (Category) {
  case (D3D12_MESSAGE_CATEGORY_MISCELLANEOUS):
    return std::string("APPLICATION_DEFINED");
    break;
  case (D3D12_MESSAGE_CATEGORY_INITIALIZATION):
    return std::string("INITIALIZATION");
    break;
  case (D3D12_MESSAGE_CATEGORY_CLEANUP):
    return std::string("CLEANUP");
    break;
  case (D3D12_MESSAGE_CATEGORY_COMPILATION):
    return std::string("COMPILATION");
    break;
  case (D3D12_MESSAGE_CATEGORY_STATE_CREATION):
    return std::string("STATE_CREATION");
    break;
  case (D3D12_MESSAGE_CATEGORY_STATE_SETTING):
    return std::string("STATE_SETTING");
    break;
  case (D3D12_MESSAGE_CATEGORY_STATE_GETTING):
    return std::string("STATE_GETTING");
    break;
  case (D3D12_MESSAGE_CATEGORY_RESOURCE_MANIPULATION):
    return std::string("RESOURCE_MANIPULATION");
    break;
  case (D3D12_MESSAGE_CATEGORY_EXECUTION):
    return std::string("EXECUTION");
    break;
  case (D3D12_MESSAGE_CATEGORY_SHADER):
    return std::string("SHADER");
    break;
  default:
    return std::string("APPLICATION_DEFINED");
    break;
  }
}

std::string
GErrors::D3D12MessageSeverityToString(D3D12_MESSAGE_SEVERITY Category) {
  switch (Category) {
  case (D3D12_MESSAGE_SEVERITY_ERROR):
    return std::string("ERROR: ");
    break;
  case (D3D12_MESSAGE_SEVERITY_WARNING):
    return std::string("WARNING: ");
    break;
  case (D3D12_MESSAGE_SEVERITY_INFO):
    return std::string("INFO: ");
    break;
  case (D3D12_MESSAGE_SEVERITY_MESSAGE):
    return std::string("MESSAGE: ");
    break;
  default:
    return std::string("CORRUPTION: ");
    break;
  }
}

void GErrors::D3D12MessageCallback(D3D12_MESSAGE_CATEGORY Category,
                                   D3D12_MESSAGE_SEVERITY Severity,
                                   D3D12_MESSAGE_ID ID, LPCSTR pDescription,
                                   void *pContext) {
  if (Severity == D3D12_MESSAGE_SEVERITY_INFO &&
      Category == D3D12_MESSAGE_CATEGORY_STATE_CREATION)
    return;
  printf("D3D12: %s %s %s \n", D3D12MessageCategoryToString(Category).c_str(),
         D3D12MessageSeverityToString(Severity).c_str(), pDescription);
  fflush(stdout);
}