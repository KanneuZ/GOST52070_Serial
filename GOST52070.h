// GOST52070.h
#pragma once

#include <cstdint>
#include <vector>
#include "ComPort.h"

class GOST52070 {
private:
	ComPort &comPort;
	uint8_t ownAddress;
	bool isBusController;
	bool verbose;

	std::vector<uint8_t> EncodeWord(uint32_t word, uint8_t syncType);
	bool DecodeWord(const std::vector<uint8_t> &bytes, uint32_t &word, uint8_t &syncType);
	static uint8_t CalculateParity(uint32_t word, int bits);

public:
	GOST52070(ComPort &port, uint8_t addr = 0x01, bool bc = true, bool v = true);

	// Создание слов
	uint32_t CreateCommandWord(uint8_t targetAddr, bool tx, uint8_t subAddr, uint8_t wordCount);
	uint32_t CreateStatusWord(bool msgError = false, bool serviceReq = false, bool groupCmd = false, bool busy = false, bool subFlag = false, bool termFlag = false);
	uint32_t CreateDataWord(uint16_t data);

	// Отправка слов
	bool SendWord(uint32_t word, uint8_t syncType);
	bool SendCommand(uint8_t targetAddr, bool tx, uint8_t subAddr, uint8_t wordCount);
	bool SendData(uint16_t data);
	bool SendStatus(bool msgError = false, bool serviceReq = false, bool groupCmd = false, bool busy = false, bool subFlag = false, bool termFlag = false);

	// Прием слов
	bool ReceiveWord(uint32_t &word, uint8_t &syncType, DWORD timeoutMs = 100);
	bool ReceiveCommand(uint32_t &cw, DWORD timeoutMs = 100);
	bool ReceiveData(uint16_t &data, DWORD timeoutMs = 100);
	bool ReceiveStatus(uint32_t &sw, DWORD timeoutMs = 100);

	// Команды управления (таблица 1)
	bool SendTransmitStatus(uint8_t targetAddr);
	bool SendBlockTransmitter(uint8_t targetAddr);
	bool SendUnblockTransmitter(uint8_t targetAddr);
	bool SendReset(uint8_t targetAddr);
	bool SendSelfTest(uint8_t targetAddr);

	// Полный цикл обмена (команда + ответ)
	bool Exchange(uint8_t targetAddr, bool tx, uint8_t subAddr, uint8_t wordCount, uint16_t *sendData = nullptr, uint16_t *recvData = nullptr, uint32_t *statusWord = nullptr, DWORD timeoutMs = 100);
};