
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
#include <SoftwareSerial.h>

#define RX_PIN A3                                            // pin usati da softwareserial
#define TX_PIN A4

SoftwareSerial debugSerial(RX_PIN, TX_PIN);

      // MODE_AM = 0x00   # Codice per AM
      // MODE_FM = 0x01   # Codice per FM
      // MODE_SSB = 0x02  # Codice per SSB

// ******************************************************************************************************************************
//
// ******************************************************************************************************************************
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

// ******************************************************************************************************************************
//
// ******************************************************************************************************************************
void IcomSim::Initialize(uint32_t frequency, uint8_t mode, uint8_t squelch) 
{
	debugSerial.begin(9600); // Inizializza la seriale software per il debug
    debugSerial.println("Debug Serial Attivata");
	
	currentFrequency = frequency;  			// Frequenza iniziale 
    currentMode = mode;              		// Modalità iniziale 
    currentSql = squelch;              		// Modalità iniziale 
}

// ******************************************************************************************************************************
//
// ******************************************************************************************************************************
void IcomSim::processCIVCommand() 
{
    static uint8_t responseBuffer[32];  // Buffer statico per accumulare i dati ricevuti (32 byte come dimensione massima)
    static uint8_t bufferIndex = 0;     // Indice per tracciare la posizione corrente nel buffer

    while (serialPort->available()) 
    {
        uint8_t byte = serialPort->read();

        // Aggiungi il byte al buffer
        if (bufferIndex < sizeof(responseBuffer))
        {
            responseBuffer[bufferIndex++] = byte;
        }

        // Se troviamo i due byte di inizio consecutivi (0xFE 0xFE), inizia un nuovo messaggio
        if (bufferIndex >= 2 &&
            responseBuffer[bufferIndex - 2] == CIV_START_BYTE &&
            responseBuffer[bufferIndex - 1] == CIV_START_BYTE)
        {
            bufferIndex = 2;  // Inizia un nuovo messaggio, mantieni solo i due byte di inizio
            // debugSerial.println("Nuovo messaggio CI-V iniziato.");
        }

        // Se troviamo il terminatore (CIV_END_BYTE), il messaggio è completo
        if (byte == CIV_END_BYTE)
        {
            if (bufferIndex >= 6)  // Un messaggio CI-V valido deve avere almeno 6 byte
            {
                uint8_t addressTo = responseBuffer[2];
                uint8_t addressFrom = responseBuffer[3];
                uint8_t command = responseBuffer[4];
                
                // Calcola la lunghezza dei dati (escludendo i byte di inizio e il terminatore)
                uint8_t dataLength = bufferIndex - 6;

                // Allocazione dinamica per i dati
                uint8_t* data = (uint8_t*)malloc(dataLength);
                if (data != nullptr)
                {
                    memcpy(data, &responseBuffer[5], dataLength);
					
					uint8_t command = responseBuffer[4];

					// Debug: stampa i dati effettivamente ricevuti
					/* Debug_Print("Dati ricevuti: ");
					for (uint8_t i = 0; i < bufferIndex; i++) {
						Debug_Print("0x%02X ", responseBuffer[i]);
					}
					Debug_Print("\n\r");
                    
                    Debug_Print("Comando : 0x%02X\n\r", command); */

                    // Elabora il comando ricevuto
                    switch (command)
                    {
						// ---------------------------------------------------- 
                        case COMMAND_SET_FREQUENCY:
                            if (dataLength >= 5)
                            {
                                uint32_t frequency = 0;

                                // Decodifica i 5 byte BCD ricevuti, invertendo l'ordine dei byte e i nibble
                                for (int i = 4; i >= 0; i--)
                                {
                                    // Inverti l'ordine dei nibble in ciascun byte
                                    uint8_t inverted_byte = (data[i] << 4) | (data[i] >> 4);

                                    uint8_t high_nibble = (inverted_byte >> 4) & 0x0F;  // Prendi la cifra più significativa (ora invertita)
                                    uint8_t low_nibble = inverted_byte & 0x0F;           // Prendi la cifra meno significativa (ora invertita)

                                    frequency = (frequency * 100) + (high_nibble * 10) + low_nibble;
                                }

                                currentFrequency = frequency;
                                frequencyChanged = true;

                                // Debug: stampa la frequenza decodificata
                                // Debug_Print("Frequenza decodificata (Hz): %ld\n\r", currentFrequency);
                            }
                            break;
							
						// ---------------------------------------------------- 	
						case COMMAND_GET_FREQUENCY:
							send_frequency(currentFrequency,addressFrom, addressTo);
							break;

						// ---------------------------------------------------- 
                        case COMMAND_SET_SQUELCH:
                            if (dataLength > 0)
                            {
                                currentSql = data[0];
                                SqlChanged = true;
                                // Debug_Print("Livello di squelch decodificato: %d\n\r", currentSql);
                            }
                            break;
							
						// ---------------------------------------------------- 	
						case COMMAND_GET_SQUELCH:
							send_squelch(currentSql, addressFrom, addressTo);
							break;	

						// ---------------------------------------------------- 
                        case COMMAND_SET_MODE:
                            if (dataLength > 0)
                            {
                                currentMode = data[0];
                                modeChanged = true;
                            }
                            break;							


						// ---------------------------------------------------- 
                        default:
                            debugSerial.println("Comando CI-V non riconosciuto.");
                            break;
                    }

                    // Libera la memoria allocata per i dati
                    free(data);
                }
                else
                {
                    debugSerial.println("Errore: memoria insufficiente per allocare i dati.");
                }
            }
            else
            {
                debugSerial.println("Messaggio CI-V non valido: troppo corto.");
            }

            // Svuota il buffer dopo aver elaborato il messaggio
            bufferIndex = 0;
        }
    }
}

// ******************************************************************************************************************************
//
// ******************************************************************************************************************************
void IcomSim::send_frequency(uint32_t frequency, uint8_t addressFrom, uint8_t addressTo)
{
    uint8_t frequencyData[5];

    // Converti la frequenza in formato BCD (Binary Coded Decimal)
    for (int i = 4; i >= 0; i--) 
    {
        frequencyData[i] = (frequency % 10) | ((frequency / 10 % 10) << 4);
        frequency /= 100;
    }

    // Invia il messaggio CI-V con la frequenza codificata
    serialPort->write(0xFE);
    serialPort->write(0xFE);
    serialPort->write(addressFrom);
    serialPort->write(addressTo);
    serialPort->write(COMMAND_GET_FREQUENCY);  	// Comando di risposta per GET_FREQUENCY

    // Invia i dati della frequenza (5 byte)
    for (int i = 0; i < 5; i++) 
    {
        serialPort->write(frequencyData[i]);
    }
    serialPort->write(0xFD);  					// Byte di fine messaggio
}	

// ******************************************************************************************************************************
//
// ******************************************************************************************************************************
void IcomSim::send_squelch(uint8_t squelch, uint8_t addressFrom, uint8_t addressTo)
{
    serialPort->write(0xFE);
    serialPort->write(0xFE);
    serialPort->write(addressFrom);
    serialPort->write(addressTo);
    serialPort->write(COMMAND_GET_SQUELCH);  // Comando di risposta

    serialPort->write(currentSql);
    serialPort->write(0xFD);  // Byte di fine messaggio
}

// ******************************************************************************************************************************
//
// ******************************************************************************************************************************
void IcomSim::sendResponse(const String& response)
{
    serialPort->println(response);
}

// ******************************************************************************************************************************
//
// ******************************************************************************************************************************
uint8_t IcomSim::isChanged()
{
    if (frequencyChanged) { frequencyChanged = false; return COMMAND_SET_FREQUENCY; }
	if (modeChanged)      { modeChanged      = false; return COMMAND_SET_MODE;      }
	if (SqlChanged)       { SqlChanged       = false; return COMMAND_SET_SQUELCH;   }
	
    return 0;
}

// ******************************************************************************************************************************
//
// ******************************************************************************************************************************
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

// ******************************************************************************************************************************
//
// ******************************************************************************************************************************
void IcomSim::Debug_Print(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char buffer[128]; // Buffer per formattare la stringa
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    debugSerial.print(buffer); // Stampa sulla seriale di debug
}