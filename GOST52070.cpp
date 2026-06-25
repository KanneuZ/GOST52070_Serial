#include "GOST52070.h"
#include <iostream>
#include <bitset>
#include <chrono>

GOST52070::GOST52070(ComPort& port, uint8_t addr, bool bc, bool v) : comPort(port), ownAddress(addr), isBusController(bc), verbose(v) {}

/* ============================================================================= */

std::vector<uint8_t> GOST52070::EncodeWord(uint32_t word, uint8_t syncType) {
	std::vector<uint8_t> bytes;
	uint8_t byte = 0;
	int bitPos = 0;

	int8_t sync[6];
	if (syncType == 0) {
		sync[0] = 1; sync[1] = -1; sync[2] = -1;
		sync[3] = 1; sync[4] = -1; sync[5] = 1;
	} else {
		sync[0] = -1; sync[1] = 1; sync[2] = 1;
		sync[3] = -1; sync[4] = 1; sync[5] = -1;
	}

	for (int i = 0; i < 6; i++) {
		if (sync[i] > 0) byte |= (1 << (7 - bitPos));
		bitPos++;
		if (bitPos == 8) {
			bytes.push_back(byte);
			byte = 0;
			bitPos = 0;
		}
	}

	for (int i = 16; i >= 0; i--) {
		bool bit = (word >> i) & 1;
		
		int8_t first = bit ? 1 : -1;
		int8_t second = bit ? -1 : 1;
		
		if (first > 0) byte |= (1 << (7 - bitPos));
		bitPos++;
		if (bitPos == 8) {
			bytes.push_back(byte);
			byte = 0;
			bitPos = 0;
		}
		
		if (second > 0) byte |= (1 << (7 - bitPos));
		bitPos++;
		if (bitPos == 8) {
			bytes.push_back(byte);
			byte = 0;
			bitPos = 0;
		}
	}

	if (bitPos > 0) bytes.push_back(byte);
	return bytes;
}

bool GOST52070::DecodeWord(const std::vector<uint8_t>& bytes, uint32_t& word, uint8_t& syncType) {
	if (bytes.size() < 5) return false;

	std::vector<int8_t> signal;
	signal.reserve(bytes.size() * 8);
	
	for (uint8_t byte : bytes) {
		for (int i = 7; i >= 0; i--) signal.push_back((byte >> i) & 1 ? 1 : -1);
	}

	if (signal.size() < 40) return false;

	// КС/ОС: + - - + - +
	if (signal[0] == 1 && signal[1] == -1 && signal[2] == -1 &&
		signal[3] == 1 && signal[4] == -1 && signal[5] == 1) {
		syncType = 0;
	}
	// СД: - + + - + -
	else if (signal[0] == -1 && signal[1] == 1 && signal[2] == 1 &&
			 signal[3] == -1 && signal[4] == 1 && signal[5] == -1) {
		syncType = 1;
	} else return false;

	word = 0;
	for (int i = 0; i < 17; ++i) {
		int idx = 6 + i * 2;
		if (idx + 1 >= (int)signal.size()) return false;
		
		// Манчестер II: 1 -> +1,-1; 0 -> -1,+1
		bool bit = (signal[idx] == 1 && signal[idx + 1] == -1);
		word = (word << 1) | (bit ? 1 : 0);
	}

	// бит четности (нечетный)
	uint8_t parityBit = word & 1;
	uint8_t calcParity = CalculateParity(word >> 1, 16);
	if (parityBit != calcParity) return false;

	return true;
}

uint8_t GOST52070::CalculateParity(uint32_t word, int bits) {
	uint8_t parity = 0;
	for (int i = 0; i < bits; ++i) parity ^= (word >> i) & 1;
	return parity ^ 1; // Нечетный
}

/* ============================================================================= */

uint32_t GOST52070::CreateCommandWord(uint8_t targetAddr, bool tx, uint8_t subAddr, uint8_t wordCount) {
	uint32_t cw = 0;
	cw |= (uint32_t)(targetAddr & 0x1F) << 11;
	if (tx) cw |= (1 << 10);
	cw |= (uint32_t)(subAddr & 0x1F) << 5;
	cw |= (wordCount & 0x1F);
	if (CalculateParity(cw, 16)) cw |= (1 << 16);
	return cw;
}

uint32_t GOST52070::CreateStatusWord(bool msgError, bool serviceReq, bool groupCmd, bool busy, bool subFlag, bool termFlag) {
	uint32_t sw = 0;
	sw |= (uint32_t)ownAddress << 11;
	if (msgError) sw |= (1 << 10);
	if (serviceReq) sw |= (1 << 9);
	if (groupCmd) sw |= (1 << 5);
	if (busy) sw |= (1 << 4);
	if (subFlag) sw |= (1 << 3);
	if (termFlag) sw |= (1 << 1);
	if (CalculateParity(sw, 16)) sw |= (1 << 16);
	return sw;
}

uint32_t GOST52070::CreateDataWord(uint16_t data) {
	uint32_t dw = (uint32_t)data << 1;
	if (CalculateParity(dw >> 1, 16)) dw |= 1;
	return dw;
}

/* ============================================================================= */

bool GOST52070::SendWord(uint32_t word, uint8_t syncType) {
	std::vector<uint8_t> bytes = EncodeWord(word, syncType);
	DWORD written = comPort.Write(bytes.data(), (DWORD)bytes.size());
	return written == bytes.size();
}

bool GOST52070::SendCommand(uint8_t targetAddr, bool tx, uint8_t subAddr, uint8_t wordCount) {
	if (!isBusController) {
		std::cerr << "Ошибка: не КШ" << std::endl;
		return false;
	}

	uint32_t cw = CreateCommandWord(targetAddr, tx, subAddr, wordCount);
	return SendWord(cw, 0);
}

bool GOST52070::SendData(uint16_t data) {
	uint32_t dw = CreateDataWord(data);
	return SendWord(dw, 1);
}

bool GOST52070::SendStatus(bool msgError, bool serviceReq, bool groupCmd, bool busy, bool subFlag, bool termFlag) {
	uint32_t sw = CreateStatusWord(msgError, serviceReq, groupCmd, busy, subFlag, termFlag);
	return SendWord(sw, 0);
}

/* ============================================================================= */

bool GOST52070::ReceiveWord(uint32_t& word, uint8_t& syncType, DWORD timeoutMs) {
	uint8_t buffer[64] = { 0 };
	DWORD bytesRead = comPort.Read(buffer, sizeof(buffer), timeoutMs);
	
	if (bytesRead == 0) return false;

	std::vector<uint8_t> bytes(buffer, buffer + bytesRead);
	return DecodeWord(bytes, word, syncType);
}

bool GOST52070::ReceiveCommand(uint32_t& cw, DWORD timeoutMs) {
	uint8_t syncType;
	if (!ReceiveWord(cw, syncType, timeoutMs)) return false;
	return (syncType == 0);
}

bool GOST52070::ReceiveData(uint16_t& data, DWORD timeoutMs) {
	uint32_t word;
	uint8_t syncType;

	if (!ReceiveWord(word, syncType, timeoutMs)) return false;
	if (syncType != 1) return false;

	data = (word >> 1) & 0xFFFF;
	return true;
}

bool GOST52070::ReceiveStatus(uint32_t& sw, DWORD timeoutMs) {
	uint8_t syncType;
	if (!ReceiveWord(sw, syncType, timeoutMs)) return false;
	return (syncType == 0);
}

/* ============================================================================= */

bool GOST52070::SendTransmitStatus(uint8_t targetAddr) {
	return SendCommand(targetAddr, false, 0x00, 0x02);
}

bool GOST52070::SendBlockTransmitter(uint8_t targetAddr) {
	return SendCommand(targetAddr, false, 0x00, 0x04);
}

bool GOST52070::SendUnblockTransmitter(uint8_t targetAddr) {
	return SendCommand(targetAddr, false, 0x00, 0x05);
}

bool GOST52070::SendReset(uint8_t targetAddr) {
	return SendCommand(targetAddr, false, 0x00, 0x08);
}

bool GOST52070::SendSelfTest(uint8_t targetAddr) {
	return SendCommand(targetAddr, false, 0x00, 0x03);
}

/* ============================================================================= */

bool GOST52070::Exchange(uint8_t targetAddr, bool tx, uint8_t subAddr, uint8_t wordCount, uint16_t *sendData, uint16_t *recvData, uint32_t *statusWord, DWORD timeoutMs) {
	if (!isBusController) {
		std::cerr << "Ошибка: не КШ" << std::endl;
		return false;
	}

	// 1. Отправляем команду
	if (!SendCommand(targetAddr, tx, subAddr, wordCount)) {
		std::cerr << "Ошибка отправки команды" << std::endl;
		return false;
	}

	// 2. Если нужно отправить данные (режим передачи)
	if (tx && sendData != nullptr) {
		for (int i = 0; i < wordCount; ++i) {
			if (!SendData(sendData[i])) {
				std::cerr << "Ошибка отправки данных" << std::endl;
				return false;
			}
		}
	}

	// 3. Принимаем ответное слово
	uint32_t sw;
	if (!ReceiveStatus(sw, timeoutMs)) {
		std::cerr << "Ответное слово не получено (таймаут)" << std::endl;
		return false;
	}

	if (statusWord != nullptr) *statusWord = sw;

	// 4. Если нужно принять данные (режим приема)
	if (!tx && recvData != nullptr) {
		for (int i = 0; i < wordCount; ++i) {
			if (!ReceiveData(recvData[i], timeoutMs)) {
				std::cerr << "Ошибка приема данных" << std::endl;
				return false;
			}
		}
	}

	return true;
}