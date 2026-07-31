#pragma once
#include "CWin.h"

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
  HrException(HRESULT hr, int line, const char *file) noexcept;
  const char *what() const noexcept override;
  const char *GetType() const noexcept override;
  HRESULT GetErrorCode() const noexcept;
  std::string GetErrorDescription() const noexcept;
  std::string TranslateErrorCode(HRESULT hr) const noexcept;

private:
  HRESULT hr;
};

class tsPrintBuffer {
  static std::vector<std::string> Buffer;
  static std::vector<std::string> Format;
  static std::mutex bufferLock;
  public:

  static void PrintFBuffered();
  static void QueuePrintF(std::string format, std::vector<std::string> str);
};