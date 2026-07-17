#pragma once
#include "CWin.h"
#include <directx/d3dx12.h>
#include <source_location>

class DLL GErrors {
public:
	struct DLL CheckerToken {
		CheckerToken() = default;
		bool operator==(const CheckerToken& other) const = default;
	};
	struct HrGrabber {
		HrGrabber(unsigned int hr, std::source_location = std::source_location::current()) noexcept;
		unsigned int hr;
		std::source_location loc;
	};
	static std::string D3D12MessageCategoryToString(D3D12_MESSAGE_CATEGORY Category){
	switch(Category){
	case(D3D12_MESSAGE_CATEGORY_MISCELLANEOUS):
		return std::string("D3D12_MESSAGE_CATEGORY_APPLICATION_DEFINED: " + std::to_string(Category));
	break;
	case(D3D12_MESSAGE_CATEGORY_INITIALIZATION):
		return std::string("D3D12_MESSAGE_CATEGORY_INITIALIZATION: " + std::to_string(Category));
	break;	
	case(D3D12_MESSAGE_CATEGORY_CLEANUP):
		return std::string("D3D12_MESSAGE_CATEGORY_CLEANUP: " + std::to_string(Category));
	break;		
	case(D3D12_MESSAGE_CATEGORY_COMPILATION):
		return std::string("D3D12_MESSAGE_CATEGORY_COMPILATION: " + std::to_string(Category));
	break;			
	case(D3D12_MESSAGE_CATEGORY_STATE_CREATION):
		return std::string("D3D12_MESSAGE_CATEGORY_STATE_CREATION: " + std::to_string(Category));
	break;				
	case(D3D12_MESSAGE_CATEGORY_STATE_SETTING):				
		return std::string("D3D12_MESSAGE_CATEGORY_STATE_SETTING: " + std::to_string(Category));
	break;
	case(D3D12_MESSAGE_CATEGORY_STATE_GETTING):
		return std::string("D3D12_MESSAGE_CATEGORY_STATE_GETTING: " + std::to_string(Category));
	break;
	case(D3D12_MESSAGE_CATEGORY_RESOURCE_MANIPULATION):
		return std::string("D3D12_MESSAGE_CATEGORY_RESOURCE_MANIPULATION: " + std::to_string(Category));
	break;				
	case(D3D12_MESSAGE_CATEGORY_EXECUTION):
		return std::string("D3D12_MESSAGE_CATEGORY_EXECUTION: " + std::to_string(Category));
	break;				
	case(D3D12_MESSAGE_CATEGORY_SHADER):
		return std::string("D3D12_MESSAGE_CATEGORY_SHADER: " + std::to_string(Category));
	break;
	default:
		return std::string("D3D12_MESSAGE_CATEGORY_APPLICATION_DEFINED: " + std::to_string(Category));
	break;
	}
}


static std::string D3D12MessageSeverityToString(D3D12_MESSAGE_SEVERITY Category){
	switch(Category){
	case(D3D12_MESSAGE_SEVERITY_ERROR):
		return std::string("D3D12_MESSAGE_CATEGORY_APPLICATION_DEFINED: " + std::to_string(Category));
	break;
	case(D3D12_MESSAGE_SEVERITY_WARNING):
		return std::string("D3D12_MESSAGE_CATEGORY_INITIALIZATION: " + std::to_string(Category));
	break;	
	case(D3D12_MESSAGE_SEVERITY_INFO):
		return std::string("D3D12_MESSAGE_CATEGORY_CLEANUP: " + std::to_string(Category));
	break;		
	case(D3D12_MESSAGE_SEVERITY_MESSAGE):
		return std::string("D3D12_MESSAGE_CATEGORY_COMPILATION: " + std::to_string(Category));
	break;			
	default:
		return std::string("D3D12_MESSAGE_SEVERITY_CORRUPTION: " + std::to_string(Category));
	break;
	}
}

static void D3D12MessageCallback(
    D3D12_MESSAGE_CATEGORY Category,
    D3D12_MESSAGE_SEVERITY Severity,
    D3D12_MESSAGE_ID ID,
    LPCSTR pDescription,
    void* pContext)
{
    printf("D3D12: %s %s %s \n", 
        D3D12MessageCategoryToString(Category).c_str(),
        D3D12MessageSeverityToString(Severity).c_str(),
        pDescription);
	fflush(stdout);
}
};

void operator >>(GErrors::HrGrabber, GErrors::CheckerToken);