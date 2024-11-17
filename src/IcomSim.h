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

#include <Arduino.h>
#include <SoftwareSerial.h>

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
#define COMMAND_SET_STEP 	  0x17
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

#define AGC_AUTO 0
#define AGC_MAN  1
#define AGC_SLOW 2
#define AGC_NOR  3
#define AGC_FAST 4

// Definizione della struttura per le variabili di stato della radio
typedef struct 
{
	uint8_t scn_port;
	uint8_t mute_port;
    uint32_t Frequency;   // Frequenza corrente (es. 145 MHz)
	uint16_t step;
    uint8_t Mode;         // Modalità corrente (FM, AM, SSB, ecc.)
	uint8_t AGC;
    uint8_t Gain;         // Guadagno RF corrente
    uint8_t Sql;          // Livello Squelch corrente
	uint8_t bw;
	uint8_t txp;
	
    union 
	{
        struct 
		{
            bool monitor:1;		// 0x0001
            bool rx:1;			// 0x0002
            bool tx:1;			// 0x0004
            bool scan:1;		// 0x0008
			bool ctcss:1;		// 0x0010
            bool dcs:1;			// 0x0020
            bool tones:1;       // 0x0040
			bool vuoto:1;		// ------	
            bool flag9:1;		// 0x0100
            bool flag10:1;		// 0x0200
            bool flag11:1;		// 0x0400
            bool flag12:1;		// 0x0800
            bool flag13:1;		// 0x1000
            bool flag14:1;		// 0x2000
            bool shortpress:1;	// 0x4000
            bool longpress:1;	// 0x8000

        } bits;  
        uint16_t Flags; 
    } Flag;
	
} VfoData_t;


#define FLAG_FREQUENCY_CHANGED   0x01  // 00000001
#define FLAG_MODE_CHANGED        0x02  // 00000010
#define FLAG_SQL_CHANGED         0x04  // 00000100
#define FLAG_GAIN_CHANGED        0x08  // 00001000  
#define FLAG_MONITOR_CHANGED     0x10  // 00010000  
#define FLAG_BW_CHANGED          0x20  // 00100000 
#define FLAG_TXP_CHANGED         0x40  // 01000000 

typedef struct
{
	bool frequencyChanged;       // Flag per indicare se la frequenza è stata modificata
    bool modeChanged;            // Flag per indicare se la modalità è stata modificata
	bool sqlChanged;             // Flag per indicare se lo squelch è stato modificato
    bool gainChanged;            // Flag per indicare se il guadagno è stato modificato
	bool monitorChanged;         // Flag per indicare che e' stato attivato o disattivato il monitor
    bool bwChanged;         	 // Flag per indicare che la bw e' cambiata
	bool txpChanged;             // Flag per indicare che la tx power e' cambiata
	
} Flags_t;

class IcomSim 
{
public:
	IcomSim(Stream& serial);
	bool Initialize(VfoData_t* initData1, VfoData_t* initData2);

	void processCIVCommand();
	
	void send_frequency(uint32_t frequency, uint8_t addressFrom, uint8_t addressTo);
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
	Flags_t Flags;  			// Variabile membro per i flag di stato
	
	void sendResponse(const String& response);
};
#endif

