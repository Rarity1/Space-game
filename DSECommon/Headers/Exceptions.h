#pragma once
#include <mutex>
#include <string>
#include <vector>


#ifndef DLL
#ifdef DESCDLL
#define DLL __declspec( dllexport )
#else
#define DLL __declspec( dllimport )
#endif
#endif

typedef long ERRORCODE;

class DLL Exceptions : public std::exception
{
public:
	Exceptions(int line, const char* file) noexcept;
	const char* what() const noexcept override;
	virtual const char* GetType() const noexcept;
	int GetLine() const noexcept;
	const std::string& GetFile() const noexcept;
	std::string GetOriginString() const noexcept;
private:
	int line;
	std::string file;
protected:
	mutable std::string whatBuffer;
};

class DLL HrException : public Exceptions {
public:
  HrException(ERRORCODE hr, int line, const char *file) noexcept;
  const char *what() const noexcept override;
  const char *GetType() const noexcept override;
  ERRORCODE GetErrorCode() const noexcept;
  std::string GetErrorDescription() const noexcept;
  std::string TranslateErrorCode(ERRORCODE hr) const noexcept;

private:
  ERRORCODE hr;
};

class tsPrintBuffer {
  static std::vector<std::string> Buffer;
  static std::vector<std::string> Format;
  static std::mutex bufferLock;
  public:

  static void PrintFBuffered();
  static void QueuePrintF(std::string format, std::vector<std::string> str);
};