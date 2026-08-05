#pragma once
#include <functional>
#include <source_location>

#ifdef __WIN32
#include <directx/d3dx12.h>
#endif

class GErrors {
public:
  struct CheckerToken {
    CheckerToken(std::function<void()> WrappedFunc = []() {}) {
      storedfunc = std::move(WrappedFunc);
    };
    std::function<void()> storedfunc;
  };
  struct ErrGrab {
    ErrGrab(unsigned int hr,
              std::source_location = std::source_location::current()) noexcept;
    unsigned int hr;
    std::source_location loc;
  };

  #ifdef __WIN32
  static std::string
  D3D12MessageCategoryToString(D3D12_MESSAGE_CATEGORY Category);
  static std::string
  D3D12MessageSeverityToString(D3D12_MESSAGE_SEVERITY Category);

  static void D3D12MessageCallback(D3D12_MESSAGE_CATEGORY Category,
                                   D3D12_MESSAGE_SEVERITY Severity,
                                   D3D12_MESSAGE_ID ID, LPCSTR pDescription,
                                   void *pContext);
  #endif
};

void operator>>(GErrors::ErrGrab, GErrors::CheckerToken);