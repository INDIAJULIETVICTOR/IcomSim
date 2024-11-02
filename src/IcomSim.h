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

#define COMMAND_GET_FREQUENCY 0x03
#define COMMAND_SET_FREQUENCY 0x05
#define COMMAND_SET_MODE 0x06
#define COMMAND_SET_SQUELCH 0x14
#define COMMAND_GET_SQUELCH 0x15

	class IcomSim 
	{
	public:
		IcomSim(Stream& serial);
		void Initialize(uint32_t frequency, uint8_t mode, uint8_t squelch);
		void processCIVCommand();
		void send_frequency(uint32_t frequency, uint8_t addressFrom, uint8_t addressTo);
		void send_squelch(uint8_t squelch, uint8_t addressFrom, uint8_t addressTo);
		void IcomSim::Debug_Print(const char *format, ...);
		
		uint8_t isChanged();
		
		uint32_t getFrequency();
		uint8_t getMode();
		uint8_t getSquelch();

	private:
		Stream* serialPort; 
		uint32_t currentFrequency;
		uint8_t currentMode;
		uint8_t currentSql;
		bool frequencyChanged;
		bool modeChanged;
		bool SqlChanged;
		void sendResponse(const String& response);
	};
#endif

