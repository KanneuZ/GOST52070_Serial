#include "ComPort.h"
#include "GOST52070.h"
#include <iostream>
#include <iomanip>

void PrintStatusWord(uint32_t sw) {
	std::cout << "Ответное слово: 0x" << std::hex << std::setw(5) << std::setfill('0') << sw << std::dec << std::setfill(' ') << std::endl;
	bool msgError = (sw >> 10) & 1;
	bool serviceReq = (sw >> 9) & 1;
	bool groupCmd = (sw >> 5) & 1;
	bool busy = (sw >> 4) & 1;
	bool subFlag = (sw >> 3) & 1;
	bool termFlag = (sw >> 1) & 1;
	std::cout << "  Ошибка в сообщении: " << (msgError ? "Да" : "Нет") << std::endl;
	std::cout << "  Запрос на обслуживание: " << (serviceReq ? "Да" : "Нет") << std::endl;
	std::cout << "  Принята групповая команда: " << (groupCmd ? "Да" : "Нет") << std::endl;
	std::cout << "  Абонент занят: " << (busy ? "Да" : "Нет") << std::endl;
	std::cout << "  Неисправность абонента: " << (subFlag ? "Да" : "Нет") << std::endl;
	std::cout << "  Неисправность ОУ: " << (termFlag ? "Да" : "Нет") << std::endl;
}

void PrintCommandWordSimple(uint32_t cw) {
	std::cout << "Командное слово: 0x" << std::hex << std::setw(5) << std::setfill('0') << cw << std::dec << std::setfill(' ') << std::endl;
	uint8_t addr = (cw >> 11) & 0x1F;
	bool tx = (cw >> 10) & 1;
	uint8_t subAddr = (cw >> 5) & 0x1F;
	uint8_t wordCount = cw & 0x1F;
	std::cout << "  Адрес ОУ: " << (int)addr << std::endl;
	std::cout << "  Прием/Передача: " << (tx ? "Передача" : "Прием") << std::endl;
	std::cout << "  Подадрес: " << (int)subAddr << std::endl;
	std::cout << "  Число СД: " << (int)wordCount << std::endl;
}

int main(int argc, char* argv[]) {
	SetConsoleOutputCP(CP_UTF8);
	
	std::cout << "ГОСТ Р 52070-2003 - Магистральный последовательный интерфейс" << std::endl;
	std::cout << "============================================================" << std::endl;

	std::string portName = "COM1";
	uint8_t ownAddress = 0x01;
	bool isBusController = true;

	if (argc > 1) portName = argv[1];
	if (argc > 2) ownAddress = (uint8_t)std::stoi(argv[2], nullptr, 16);
	if (argc > 3) isBusController = (std::stoi(argv[3]) != 0);

	std::cout << "Порт: " << portName << std::endl;
	std::cout << "Адрес ОУ: 0x" << std::hex << (int)ownAddress << std::dec << std::endl;
	std::cout << "Режим: " << (isBusController ? "КШ" : "ОУ") << std::endl;

	ComPort comPort;
	if (!comPort.Open(portName)) {
		std::cerr << "Не удалось открыть порт" << std::endl;
		std::cout << "\nИспользование: " << argv[0] << " [COM-порт] [адрес ОУ hex] [КШ(1)/ОУ(0)]" << std::endl;
		std::cout << "Пример: " << argv[0] << " COM3 01 1" << std::endl;
		return 1;
	}

	GOST52070 gost(comPort, ownAddress, isBusController, true);

	std::cout << "\nКоманды:\n"
			  << "  1 - Отправить команду (КШ)\n"
			  << "  2 - Отправить данные\n"
			  << "  3 - Отправить ответное слово (ОУ)\n"
			  << "  4 - Отправить КУ 'Передать ОС'\n"
			  << "  5 - Отправить КУ 'Сброс'\n"
			  << "  6 - Отправить КУ 'Блокировать передатчик'\n"
			  << "  7 - Отправить КУ 'Разблокировать передатчик'\n"
			  << "  8 - Отправить КУ 'Начать самоконтроль'\n"
			  << "  9 - Принять слово\n"
			  << " 10 - Полный цикл обмена (КШ)\n"
			  << "  0 - Выход\n" << std::endl;

	int cmd;
	while (true) {
		std::cout << "> ";
		std::cin >> cmd;

		if (cmd == 0) break;

		switch (cmd) {
			case 1: {
				uint32_t addr, subAddr, count;
				bool tx;
				std::cout << "Адрес ОУ (0-31): ";
				std::cin >> addr;
				std::cout << "Прием/Передача (0/1): ";
				std::cin >> tx;
				std::cout << "Подадрес (0-31): ";
				std::cin >> subAddr;
				std::cout << "Число СД (0-31): ";
				std::cin >> count;
				
				if (gost.SendCommand((uint8_t)addr, tx, (uint8_t)subAddr, (uint8_t)count)) {
					std::cout << "Команда отправлена" << std::endl;
				}
				break;
			}
			case 2: {
				uint16_t data;
				std::cout << "Данные (hex): ";
				std::cin >> std::hex >> data >> std::dec;
				if (gost.SendData(data)) {
					std::cout << "Данные отправлены" << std::endl;
				}
				break;
			}
			case 3: {
				bool msgErr, srvReq, busy, subFlag, termFlag;
				std::cout << "Ошибка в сообщении (0/1): ";
				std::cin >> msgErr;
				std::cout << "Запрос на обслуживание (0/1): ";
				std::cin >> srvReq;
				std::cout << "Абонент занят (0/1): ";
				std::cin >> busy;
				std::cout << "Неисправность абонента (0/1): ";
				std::cin >> subFlag;
				std::cout << "Неисправность ОУ (0/1): ";
				std::cin >> termFlag;
				
				if (gost.SendStatus(msgErr, srvReq, false, busy, subFlag, termFlag)) {
					std::cout << "Ответное слово отправлено" << std::endl;
				}
				break;
			}
			case 4: {
				uint32_t addr;
				std::cout << "Адрес ОУ (0-31): ";
				std::cin >> addr;
				if (gost.SendTransmitStatus((uint8_t)addr)) {
					std::cout << "КУ 'Передать ОС' отправлена" << std::endl;
				}
				break;
			}
			case 5: {
				uint32_t addr;
				std::cout << "Адрес ОУ (0-31): ";
				std::cin >> addr;
				if (gost.SendReset((uint8_t)addr)) {
					std::cout << "КУ 'Сброс' отправлена" << std::endl;
				}
				break;
			}
			case 6: {
				uint32_t addr;
				std::cout << "Адрес ОУ (0-31): ";
				std::cin >> addr;
				if (gost.SendBlockTransmitter((uint8_t)addr)) {
					std::cout << "КУ 'Блокировать передатчик' отправлена" << std::endl;
				}
				break;
			}
			case 7: {
				uint32_t addr;
				std::cout << "Адрес ОУ (0-31): ";
				std::cin >> addr;
				if (gost.SendUnblockTransmitter((uint8_t)addr)) {
					std::cout << "КУ 'Разблокировать передатчик' отправлена" << std::endl;
				}
				break;
			}
			case 8: {
				uint32_t addr;
				std::cout << "Адрес ОУ (0-31): ";
				std::cin >> addr;
				if (gost.SendSelfTest((uint8_t)addr)) {
					std::cout << "КУ 'Начать самоконтроль' отправлена" << std::endl;
				}
				break;
			}
			case 9: {
				uint32_t word;
				uint8_t syncType;
				std::cout << "Ожидание приема..." << std::endl;
				if (gost.ReceiveWord(word, syncType, 1000)) {
					std::cout << "Получено слово: 0x" << std::hex << std::setw(5) << std::setfill('0') << word << std::dec << std::setfill(' ') << std::endl;
					std::cout << "Тип: " << (syncType == 0 ? "КС/ОС" : "СД") << std::endl;
					
					if (syncType == 0) {
						uint8_t addr = (word >> 11) & 0x1F;
						if (addr == ownAddress || addr == 0x1F) {
							PrintCommandWordSimple(word);
						} else {
							PrintStatusWord(word);
						}
					} else {
						uint16_t data = (word >> 1) & 0xFFFF;
						std::cout << "Данные: 0x" << std::hex << data << std::dec << std::endl;
					}
				} else {
					std::cout << "Слово не получено (таймаут)" << std::endl;
				}
				break;
			}
			case 10: {
				if (!isBusController) {
					std::cout << "Эта команда доступна только в режиме КШ" << std::endl;
					break;
				}
				uint32_t addr, subAddr, count;
				bool tx;
				std::cout << "Адрес ОУ (0-31): ";
				std::cin >> addr;
				std::cout << "Прием/Передача (0=прием, 1=передача): ";
				std::cin >> tx;
				std::cout << "Подадрес (0-31): ";
				std::cin >> subAddr;
				std::cout << "Число СД (0-31): ";
				std::cin >> count;

				uint16_t sendData[32] = {0};
				uint16_t recvData[32] = {0};

				if (tx) {
					std::cout << "Введите " << count << " слов данных (hex): ";
					for (uint32_t i = 0; i < count; ++i) {
						std::cin >> std::hex >> sendData[i] >> std::dec;
					}
				}

				uint32_t statusWord;
				if (gost.Exchange((uint8_t)addr, tx, (uint8_t)subAddr, (uint8_t)count,
								  tx ? sendData : nullptr,
								  !tx ? recvData : nullptr,
								  &statusWord, 1000)) {
					std::cout << "Обмен успешно завершен" << std::endl;
					PrintStatusWord(statusWord);
					
					if (!tx) {
						std::cout << "Полученные данные: ";
						for (uint32_t i = 0; i < count; ++i) {
							std::cout << "0x" << std::hex << recvData[i] << " " << std::dec;
						}
						std::cout << std::endl;
					}
				} else {
					std::cout << "Ошибка обмена" << std::endl;
				}
				break;
			}
			default:
				std::cout << "Неизвестная команда" << std::endl;
		}
	}

	comPort.Close();
	std::cout << "Программа завершена" << std::endl;
	return 0;
}