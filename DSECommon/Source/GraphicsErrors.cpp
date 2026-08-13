#include "GraphicsErrors.h"
#include "Exceptions.h"
#include <ranges>


#if defined(_WIN32)
#include "CWin.h"
#include <directx/d3dx12.h>
#endif


void operator>>(GErrors Grabber, GErrors::CheckerToken tkn) {
  tkn.storedfunc();
  if (FAILED(Grabber.hr)) {
    std::string err = Grabber.TranslateErrorCode(Grabber.hr);
    std::print("Graphics ERROR: {} {} \n", err.c_str(), Grabber.GetOriginString().c_str());
    throw Grabber;
  } else if(Grabber.hr != S_OK){
    // get error description as narrow string with crlf removed
    std::string Message = Grabber.TranslateErrorCode(Grabber.hr);
    std::print("Graphics INFO: {} {} \n", Message.c_str(), Grabber.GetOriginString().c_str());
  }
};


#ifdef _WIN32
std::string
D3D12MessageCategoryToString(D3D12_MESSAGE_CATEGORY Category) {
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
D3D12MessageSeverityToString(D3D12_MESSAGE_SEVERITY Category) {
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

void D3D12MessageCallback(D3D12_MESSAGE_CATEGORY Category,
                                   D3D12_MESSAGE_SEVERITY Severity,
                                   D3D12_MESSAGE_ID ID, LPCSTR pDescription,
                                   void *pContext) {
  if (Severity == D3D12_MESSAGE_SEVERITY_INFO &&
      Category == D3D12_MESSAGE_CATEGORY_STATE_CREATION)
    return;
  std::print("D3D12: {} {} {} \n", D3D12MessageCategoryToString(Category).c_str(),
         D3D12MessageSeverityToString(Severity).c_str(), pDescription);
}
#endif