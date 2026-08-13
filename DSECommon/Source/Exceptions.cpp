#include "Exceptions.h"
#include <iostream>
#include <sstream>
#include <string>


#if defined(_WIN32)
#include <CWin.h>
#endif





const char* Exceptions::what() const noexcept {
	return whatBuffer.c_str();
}
const char* Exceptions::GetType() const noexcept
{
	return "String Based";
}

int Exceptions::GetLine() const noexcept
{
	return source.line();
}

const std::string Exceptions::GetFile() const noexcept
{
	return source.file_name();
}

std::string Exceptions::GetOriginString() const noexcept
{
	std::ostringstream oss;
	oss << "[File] " << GetFile() << '\n'
		<< " [Line] " << GetLine();
	return oss.str();
}

//Windows Exceptions
HrException::HrException(ERRORCODE hr, std::source_location src)noexcept:Exceptions(TranslateErrorCode(hr).c_str(), src),hr(hr){};

std::string HrException::TranslateErrorCode(ERRORCODE hr) const noexcept
{
	char* pMsgBuf = nullptr;
	// windows will allocate memory for err string and make our pointer point to it
	const DWORD nMsgLen = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&pMsgBuf), 0, nullptr
	);
	// 0 string length returned indicates a failure
	if (nMsgLen == 0)
	{
		return "Unidentified error code";
	}
	// copy error string from windows-allocated buffer to std::string
	std::string errorString = pMsgBuf;
	// free windows buffer
	LocalFree(pMsgBuf);
	return errorString;
}

const char* HrException::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << '\n'
		<< "[Error Code] 0x" << std::hex << std::uppercase << GetErrorCode() << '\n'
		<< "[Description] " << what() << '\n'
		<< GetOriginString();
    strBuffer = oss.str();
    return strBuffer.c_str();
}

const char *HrException::GetType() const noexcept {
  return "HR Exception";
}
ERRORCODE HrException::GetErrorCode() const noexcept { return hr; }

std::queue<std::function<void()>> tsPrintBuffer::Queue;
std::mutex tsPrintBuffer::bufferLock;
void tsPrintBuffer::PrintFBuffered() {
  bufferLock.lock();
  while(!Queue.empty()){
    Queue.front()();
    Queue.pop();
  }
  fflush(stdout);
  bufferLock.unlock();
}


void operator>>(Exceptions Grabber, Exceptions::CheckerToken tkn) {
  if ((strcmp(Grabber.what(), "ERROR:") > 0)) {
    std::string err = Grabber.what();
    printf("%s := \n  %s \n", err.c_str(), Grabber.GetOriginString().c_str());
    fflush(stdout);
    throw Grabber;
  } else{
    std::string mssg = Grabber.what();
    std::printf("%s := \n  %s \n", mssg.c_str(), Grabber.GetOriginString().c_str());

  }
};

void operator>>(HrException Grabber, Exceptions::CheckerToken tkn) {
  if (FAILED(Grabber.hr)) {
    std::string err = Grabber.TranslateErrorCode(Grabber.hr);
    printf("ERROR: %s \n %s \n Error Code: %ld \n", err.c_str(), Grabber.GetOriginString().c_str(), Grabber.hr);
    fflush(stdout);
    throw Grabber;
  } else if (Grabber.hr != S_OK) {
    // get error description as narrow string with crlf removed
    std::string Message = Grabber.TranslateErrorCode(Grabber.hr);
    printf("INFO: %s \n %s \n", Message.c_str(), Grabber.GetOriginString().c_str());
    fflush(stdout);
  }
};