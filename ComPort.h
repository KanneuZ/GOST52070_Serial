#pragma once
#include <windows.h>
#include <string>

class ComPort {
private:
	HANDLE hPort;

public:
	ComPort();
	~ComPort();

	bool Open(const std::string& portName, DWORD baudRate = 1000000);
	void Close();
	bool IsOpen() const { return hPort != INVALID_HANDLE_VALUE; }

	DWORD Write(const uint8_t* data, DWORD size);
	DWORD Read(uint8_t* buffer, DWORD size, DWORD timeoutMs = 100);
	void Flush();
};