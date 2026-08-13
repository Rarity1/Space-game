#pragma once
#include <cstdio>
#include <functional>
#include <functional>
#include <queue>
#include <source_location>
#include <mutex>
#include <string>
#include <format>
#include <tuple>
#include <utility>
#include <print>

#ifndef DLL
#ifdef DESCDLL
#define DLL __declspec( dllexport )
#else
#define DLL __declspec( dllimport )
#endif
#endif

typedef long ERRORCODE;


class DLL tsPrintBuffer {
  private:
  static std::queue<std::function<void()>> Queue;
  static std::mutex bufferLock;
  public:
  static void PrintFBuffered();
  template<typename... _Args>
  static inline void QueuePrintF( std::format_string<_Args...> format, _Args... Arguments) {
  bufferLock.lock();
  Queue.emplace([format, Arguments...]()mutable{
    std::print(stdout, format, std::forward<_Args>(Arguments)...);
  });
  bufferLock.unlock();
}
};

class DLL Exceptions : public std::exception
{
public:
	Exceptions(const char * str, std::source_location src = std::source_location::current())noexcept:source(src),whatBuffer(str){};
	const char* what() const noexcept override;
	virtual const char* GetType() const noexcept;
	int GetLine() const noexcept;
	const std::string GetFile() const noexcept;
	std::string GetOriginString() const noexcept;
  const std::source_location source;
  struct CheckerToken {
    CheckerToken(std::function<void()> WrappedFunc = []() {}) {
      storedfunc = std::move(WrappedFunc);
    };
    std::function<void()> storedfunc;
  };
protected:
	const std::string whatBuffer;
};

class DLL HrException : public Exceptions {
public:
  HrException(ERRORCODE hr, std::source_location src = std::source_location::current()) noexcept;
  const char *what() const noexcept override;
  const char *GetType() const noexcept override;
  ERRORCODE GetErrorCode() const noexcept;
  std::string TranslateErrorCode(ERRORCODE hr) const noexcept;
  const ERRORCODE hr;
private:
  mutable std::string strBuffer;
};



void operator>>(Exceptions, Exceptions::CheckerToken);
void operator>>(HrException, Exceptions::CheckerToken);