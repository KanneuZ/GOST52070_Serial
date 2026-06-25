#include "ComPort.h"
#include <iostream>

ComPort::ComPort() : hPort(INVALID_HANDLE_VALUE) {}

ComPort::~ComPort() {
	Close();
}

bool ComPort::Open(const std::string& portName, DWORD baudRate) {
	if (IsOpen()) Close();

	std::string fullPortName = "\\\\.\\" + portName;
	hPort = CreateFileA(
		fullPortName.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, 0, NULL
	);

	if (hPort == INVALID_HANDLE_VALUE) {
		std::cerr << "Ошибка открытия порта " << portName << std::endl;
		return false;
	}

	DCB dcb = { 0 };
	dcb.DCBlength = sizeof(DCB);

	if (!GetCommState(hPort, &dcb)) {
		std::cerr << "Ошибка получения состояния порта" << std::endl;
		Close();
		return false;
	}

	dcb.BaudRate = baudRate;
	dcb.ByteSize = 8;
	dcb.StopBits = ONESTOPBIT;
	dcb.Parity = NOPARITY;
	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;
	dcb.fBinary = TRUE;
	dcb.fParity = FALSE;

	if (!SetCommState(hPort, &dcb)) {
		std::cerr << "Ошибка установки параметров порта" << std::endl;
		Close();
		return false;
	}

	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout = 10;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.ReadTotalTimeoutConstant = 100;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 100;

	if (!SetCommTimeouts(hPort, &timeouts)) {
		std::cerr << "Ошибка установки таймаутов" << std::endl;
		Close();
		return false;
	}

	PurgeComm(hPort, PURGE_RXCLEAR | PURGE_TXCLEAR);
	std::cout << "Порт " << portName << " открыт" << std::endl;
	return true;
}

void ComPort::Close() {
	if (hPort != INVALID_HANDLE_VALUE) {
		CloseHandle(hPort);
		hPort = INVALID_HANDLE_VALUE;
	}
}

DWORD ComPort::Write(const uint8_t* data, DWORD size) {
	if (!IsOpen()) return 0;
	DWORD bytesWritten = 0;
	WriteFile(hPort, data, size, &bytesWritten, NULL);
	return bytesWritten;
}

DWORD ComPort::Read(uint8_t* buffer, DWORD size, DWORD timeoutMs) {
	if (!IsOpen()) return 0;

	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout = timeoutMs;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = timeoutMs;
	SetCommTimeouts(hPort, &timeouts);

	DWORD bytesRead = 0;
	ReadFile(hPort, buffer, size, &bytesRead, NULL);
	return bytesRead;
}

void ComPort::Flush() {
	if (IsOpen()) PurgeComm(hPort, PURGE_RXCLEAR | PURGE_TXCLEAR);
}