CLOPT = -c -Wall -O2 -std=c++17
LIBS = 

all: GOST52070_Serial

GOST52070_Serial.o:			GOST52070_Serial.cpp ComPort.h GOST52070.h
				g++ $(CLOPT) GOST52070_Serial.cpp

ComPort.o:		ComPort.cpp ComPort.h
				g++ $(CLOPT) ComPort.cpp

GOST52070.o:	GOST52070.cpp GOST52070.h ComPort.h
				g++ $(CLOPT) GOST52070.cpp

GOST52070_Serial:		GOST52070_Serial.o ComPort.o GOST52070.o
				g++ -o GOST52070_Serial GOST52070_Serial.o ComPort.o GOST52070.o $(LIBS)