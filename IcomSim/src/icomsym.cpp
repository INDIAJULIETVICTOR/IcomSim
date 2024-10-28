
/*
 * Project Name: Radio Firmware
 * File: IcomSYM.cpp
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





#include "IcomSim.h"

      // MODE_AM = 0x00   # Codice per AM
      // MODE_FM = 0x01   # Codice per FM
      // MODE_SSB = 0x02  # Codice per SSB

IcomSim::IcomSim(Stream& serial)
{
    serialPort = &serial;
    currentFrequency = 145000000UL;  	// Frequenza iniziale 145 MHz
    currentMode = 1;              		// Modalità iniziale FM
    currentSql = 1;              		// Modalità iniziale Squelch
    frequencyChanged = false;
    modeChanged = false;
    SqlChanged = false;
}

void IcomSim::processCIVCommand() 
{
	uint32_t frequency = 0;
	uint8_t frequencyData[5];
	
    if (serialPort->available()) 
	{
        uint8_t startByte1 = serialPort->read();
        if (startByte1 == 0xFE) 
		{
            uint8_t startByte2 = serialPort->read();
            if (startByte2 == 0xFE) 
			{
                uint8_t addressTo = serialPort->read();
                uint8_t addressFrom = serialPort->read();
                uint8_t command = serialPort->read();

				switch(command)
				{
					case COMMAND_SET_FREQUENCY :
						for (int i = 0; i < 5; i++) 
						{
							frequencyData[i] = serialPort->read();
						}
						frequency = 0;
						for (int i = 0; i < 5; i++) 
						{
							frequency = (frequency * 100) + ((frequencyData[i] >> 4) * 10) + (frequencyData[i] & 0x0F);
						}
						currentFrequency = frequency;
						frequencyChanged = true;

						// Converti la frequenza in una stringa ASCII senza alcun testo aggiuntivo e inviala solo una volta
						// String frequencyStr = String(frequency);
						// sendResponse(frequencyStr);
						break;
						
					case COMMAND_GET_FREQUENCY:
						frequency = currentFrequency;
						
						for (int i = 4; i >= 0; i--) 
						{
							frequencyData[i] = (frequency % 10) | ((frequency / 10 % 10) << 4);
							frequency /= 100;
						}
						serialPort->write(0xFE);
						serialPort->write(0xFE);
						serialPort->write(addressFrom);
						serialPort->write(addressTo);
						serialPort->write(0x03); // Response command
						for (int i = 0; i < 5; i++) 
						{
							serialPort->write(frequencyData[i]);
						}
						serialPort->write(0xFD);
						break;
											
                    case COMMAND_SET_MODE:
                        if (serialPort->available()) 
                        {
                            uint8_t mode = serialPort->read();
                            currentMode = mode;  
                            modeChanged = true;
                        }
                        break;

                    case COMMAND_SET_SQUELCH:
                        if (serialPort->available()) 
                        {
                            uint8_t mode = serialPort->read();
                            currentSql = mode;  
                            SqlChanged = true;
                        }
                        break;
						
				}

                uint8_t endByte = serialPort->read();
                if (endByte != 0xFD) 
				{

                }
            }
        }
    }
}


void IcomSim::sendResponse(const String& response)
{
    serialPort->println(response);
}

uint8_t IcomSim::isChanged()
{
    if (frequencyChanged) { frequencyChanged = false; return COMMAND_SET_FREQUENCY; }
	if (modeChanged)      { modeChanged      = false; return COMMAND_SET_MODE;      }
	if (SqlChanged)       { SqlChanged       = false; return COMMAND_SET_SQUELCH;   }
	
    return 0;
}

uint8_t IcomSim::getMode()
{
	return currentMode;
}

uint8_t IcomSim::getSquelch()
{
	return currentSql;
}

uint32_t IcomSim::getFrequency()
{
	return currentFrequency;
}
