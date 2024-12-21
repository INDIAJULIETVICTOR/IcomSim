/*
 * Project Name: Radio Firmware
 * File: IcomSYM.h
 *
 * Copyright (C) 2024 Fabrizio Palumbo (IU0IJV)
 * 
 * This program is distributed under the terms of the MIT license.
 * You can obtain a copy of the license at:
 * https://opensource.org/licenses/MIT
 *
 * DESCRIPTION:
 * Library implementation for managing the CI-V protocol.
 *
 * AUTHOR: Fabrizio Palumbo
 * CREATION DATE: October 27, 2024
 *
 * CONTACT: t.me/IU0IJV
 *
 * NOTES:
 * - This implementation allows communication using the Icom CI-V protocol for remote control of compatible radios.
 */

#ifndef ICOMSIM_H
#define ICOMSIM_H

#ifdef ESP32
    #include <HardwareSerial.h>

#else
	#include <Arduino.h>
    #include <SoftwareSerial.h>

#endif

#include "BK4819.h"

#define CIV_START_BYTE 0xFE
#define CIV_END_BYTE 0xFD

#define CIV_ADDRESS_RADIO 0xE0
#define CIV_ADDRESS_COMPUTER 0x00

#define COMMAND_GET_FREQUENCY 0x03
#define COMMAND_SET_FREQUENCY 0x05
#define COMMAND_SET_MODE      0x06
#define COMMAND_SET_SQUELCH   0x14
#define COMMAND_GET_SQUELCH   0x15
#define COMMAND_SET_AGC  	  0x16

#define COMMAND_SET_SCAN 	  0x18
#define COMMAND_GET_RSSI 	  0x19
#define COMMAND_SET_MONITOR   0x1A
#define COMMAND_SET_RFGAIN 	  0x1C
#define COMMAND_GET_RFGAIN 	  0x1D
#define COMMAND_SET_BANDWIDTH 0x1E
#define COMMAND_GET_BANDWIDTH 0x1F
#define COMMAND_SET_TX_POWER  0x20
#define COMMAND_GET_TX_POWER  0x21
#define COMMAND_GET_STATUS	  0x22	

#define COMMAND_SET_STEP 	  0x23
#define COMMAND_GET_STEP 	  0x24

#define AGC_AUTO 0
#define AGC_MAN  1
#define AGC_SLOW 2
#define AGC_NOR  3
#define AGC_FAST 4



#define FLAG_FREQUENCY_CHANGED   0x0001  // 0000000000000001
#define FLAG_MODE_CHANGED        0x0002  // 0000000000000010
#define FLAG_SQL_CHANGED         0x0004  // 0000000000000100
#define FLAG_GAIN_CHANGED        0x0008  // 0000000000001000  
#define FLAG_MONITOR_CHANGED     0x0010  // 0000000000010000  
#define FLAG_BW_CHANGED          0x0020  // 0000000000100000 
#define FLAG_TXP_CHANGED         0x0040  // 0000000001000000 
#define FLAG_STEP_CHANGED        0x0080  // 0000000010000000

typedef struct
{
    union {
        struct {
            bool frequencyChanged:1;
            bool modeChanged:1;
            bool sqlChanged:1;
            bool gainChanged:1;
            bool monitorChanged:1;
            bool bwChanged:1;
            bool txpChanged:1;
            bool stepChanged:1;

            bool vuoto1:1;
            bool vuoto2:1;
            bool vuoto3:1;
            bool vuoto4:1;
            bool vuoto5:1;
            bool vuoto6:1;
            bool vuoto7:1;
            bool vuoto8:1;
        };
        uint16_t All;  // Rappresentazione combinata come uint16_t
    };
} Flags_t;

class IcomSim 
{
public:
	IcomSim(Stream& serial);
	bool Initialize(VfoData_t* initData1, VfoData_t* initData2);

	void processCIVCommand();
	
	void send_frequency(uint8_t comand, uint32_t frequency, uint8_t addressFrom, uint8_t addressTo);
	void send_rssi(uint16_t rssi, uint8_t addressFrom, uint8_t addressTo);
	void send_status(uint8_t vfo, uint8_t addressFrom, uint8_t addressTo);
	void send_command(uint8_t command, uint8_t value, uint8_t addressFrom, uint8_t addressTo);
	
	void sendToSerial(const uint8_t* data, size_t length);
	
	void processSerialQueue();
	
	void Debug_Print(const char *format, ...);
	
	uint16_t isChanged();

private:
	Stream* serialPort; 
	VfoData_t* VfoData[2];  	// Variabile membro per i dati della radio
	Flags_t Flags;  				// Variabile membro per i flag di stato
	
	void sendResponse(const String& response);
};
#endif

