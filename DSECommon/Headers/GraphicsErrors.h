#pragma once
#include "Exceptions.h"

class HrException;
class GErrors : public HrException {
public:
  GErrors(ERRORCODE hr, std::source_location src = std::source_location::current()):HrException(hr, src){};
  struct CheckerToken {
    CheckerToken(std::function<void()> WrappedFunc = []() {}) {
      storedfunc = std::move(WrappedFunc);
    };
    std::function<void()> storedfunc;
  };

};

void operator>>(GErrors, GErrors::CheckerToken);