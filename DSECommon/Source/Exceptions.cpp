#include "Exceptions.h"
#include <sstream>


#if defined(_WIN32)
#include <CWin.h>
#endif

Exceptions::Exceptions(int line, const char* file) noexcept
	:
	line(line),
	file(file) {
}



const char* Exceptions::what() const noexcept {
	std::ostringstream oss;
	oss << GetType() << '\n'
		<< GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();

}
const char* Exceptions::GetType() const noexcept
{
	return "My Exception";
}

int Exceptions::GetLine() const noexcept
{
	return line;
}

const std::string& Exceptions::GetFile() const noexcept
{
	return file;
}

std::string Exceptions::GetOriginString() const noexcept
{
	std::ostringstream oss;
	oss << "[File] " << file << '\n'
		<< "[Line] " << line;
	return oss.str();
}

//Window Exception
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
HrException::HrException(ERRORCODE hr, int line, const char* file) noexcept
	:
	Exceptions(line, file),
	hr(hr)
{
}
const char* HrException::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << '\n'
		<< "[Error Code] 0x" << std::hex << std::uppercase << GetErrorCode() << '\n'
		<< "[Description] " << GetErrorDescription() << '\n'
		<< GetOriginString();
        whatBuffer = oss.str();
        return whatBuffer.c_str();
}
const char *HrException::GetType() const noexcept {
  return "Demo Window Exception";
}
ERRORCODE HrException::GetErrorCode() const noexcept { return hr; }
std::string HrException::GetErrorDescription() const noexcept {
  return TranslateErrorCode(hr);
}




std::vector<std::string> tsPrintBuffer::Buffer = {{""}};
std::vector<std::string> tsPrintBuffer::Format = {{""}};
std::mutex tsPrintBuffer::bufferLock;
void tsPrintBuffer::PrintFBuffered() {
  bufferLock.lock();
  auto b = Buffer.begin();
  for(auto f : Format){
    printf(f.c_str(), b->c_str());
    b++;
  }
  
  fflush(stdout);
  Format =  {{""}};
  Buffer =  {{""}};
  bufferLock.unlock();
}
void tsPrintBuffer::QueuePrintF(std::string format, std::vector<std::string> str) {
  bufferLock.lock();
  Buffer.append_range(str);
  Format.emplace_back(format);
  bufferLock.unlock();
}