
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

// Costruttore della classe IcomSim
IcomSim::IcomSim(Stream& serial)
{
    serialPort = &serial;

    // Inizializza le variabili membro Vfodata e Flags a zero
    memset(&VfoData, 0, sizeof(VfoData_t));
    memset(&Flags, 0, sizeof(Flags_t));
}


// ******************************************************************************************************************************
// Funzione di inizializzazione che accetta una struttura di dati iniziale
// ******************************************************************************************************************************
void IcomSim::Initialize(const VfoData_t& initData)
{
    debugSerial.begin(9600); // Inizializza la seriale software per il debug
    debugSerial.println("Debug Serial Attivata");

    // Assegna direttamente la struttura di dati iniziale alla variabile membro Vfodata
    VfoData = initData;

    // Resetta tutti i flag 
    memset(&Flags, 0, sizeof(Flags_t));
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

                                VfoData.Frequency = frequency;
                                Flags.frequencyChanged = true;

                                // Debug: stampa la frequenza decodificata
                                // Debug_Print("Frequenza decodificata (Hz): %ld\n\r", currentFrequency);
                            }
                            break;
							
						// ---------------------------------------------------- 	
						case COMMAND_GET_FREQUENCY:
							send_frequency(VfoData.Frequency,addressFrom, addressTo);
							break;

						// ---------------------------------------------------- 
                        case COMMAND_SET_SQUELCH:
                            if (dataLength > 0)
                            {
                                VfoData.Sql = data[0];
                                Flags.sqlChanged = true;
                                // Debug_Print("Livello di squelch decodificato: %d\n\r", currentSql);
                            }
                            break;
							
						// ---------------------------------------------------- 	
						case COMMAND_GET_SQUELCH:
							send_squelch(VfoData.Sql, addressFrom, addressTo);
							break;	

						// ---------------------------------------------------- 
                        case COMMAND_SET_MODE:
                            if (dataLength > 0)
                            {
                               VfoData.Mode = data[0];
                               Flags.modeChanged = true;
                            }
                            break;							

						// ---------------------------------------------------- 
                        case COMMAND_SET_RFGAIN:
						    // Debug_Print("%d\r\n",data[0]);
                            if (dataLength > 0)
                            {
                                VfoData.Gain = data[0];
                                Flags.gainChanged = true;
                            }
                            break;
							
						// ---------------------------------------------------- 	
						case COMMAND_GET_RFGAIN:
							send_rfgain(VfoData.Gain, addressFrom, addressTo);
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
    uint8_t message[15];
    message[0] = 0xFE;
    message[1] = 0xFE;
    message[2] = addressFrom;
    message[3] = addressTo;
    message[4] = COMMAND_GET_FREQUENCY;  			// Comando di risposta per GET_FREQUENCY

    // Converti la frequenza in formato BCD (Binary Coded Decimal)
    for (int i = 4; i >= 0; i--) 
    {
        message[5+i] = (frequency % 10) | ((frequency / 10 % 10) << 4);
        frequency /= 100;
    }

    message[10] = 0xFD;  							// Byte di fine messaggio

    sendToSerial(message, sizeof(message));			// Invia il messaggio usando la funzione centralizzata
}


// ******************************************************************************************************************************
//
// ******************************************************************************************************************************
void IcomSim::send_squelch(uint8_t squelch, uint8_t addressFrom, uint8_t addressTo)
{
    uint8_t message[7];
    message[0] = 0xFE;
    message[1] = 0xFE;
    message[2] = addressFrom;
    message[3] = addressTo;
    message[4] = COMMAND_GET_SQUELCH;  				// Comando di risposta per GET_SQUELCH
    message[5] = squelch;
    message[6] = 0xFD;  							// Byte di fine messaggio

    sendToSerial(message, sizeof(message));			// Invia il messaggio usando la funzione centralizzata
}



// ******************************************************************************************************************************
//
// ******************************************************************************************************************************
void IcomSim::send_rssi(uint16_t rssi, uint8_t addressFrom, uint8_t addressTo)
{
    uint8_t message[8];
    message[0] = 0xFE;
    message[1] = 0xFE;
    message[2] = addressFrom;
    message[3] = addressTo;
    message[4] = COMMAND_GET_RSSI;  				// Comando di risposta
													// Dividi il valore RSSI in due byte (LSB e MSB)
    message[5] = rssi & 0xFF;       				// Byte meno significativo
    message[6] = (rssi >> 8) & 0xFF; 				// Byte più significativo

    message[7] = 0xFD;  							// Byte di fine messaggio

    sendToSerial(message, sizeof(message));			// Invia il messaggio usando la funzione centralizzata
}


// ******************************************************************************************************************************
//
// ******************************************************************************************************************************
void IcomSim::send_rfgain(uint8_t rfgain, uint8_t addressFrom, uint8_t addressTo)
{
    uint8_t message[7];
    message[0] = 0xFE;
    message[1] = 0xFE;
    message[2] = addressFrom;
    message[3] = addressTo;
    message[4] = COMMAND_GET_RFGAIN;  				// Comando di risposta per GET_RFGAIN
    message[5] = rfgain & 0xFF;
    message[6] = 0xFD;  							// Byte di fine messaggio

    sendToSerial(message, sizeof(message));			// Invia il messaggio usando la funzione centralizzata
}


// ******************************************************************************************************************************
//
// ******************************************************************************************************************************

volatile bool serialInUse = false;

void IcomSim::sendToSerial(const uint8_t* data, size_t length)
{
												// Definisci il timeout in microsecondi
    const unsigned long timeout = 100000UL; 	// 100 millisecondi
    unsigned long startTime = micros();
												// Attendi il lock con timeout
    while (serialInUse) 
	{
        if ((micros() - startTime) > timeout) 
		{
			serialInUse = false;				// Rilascia il lock
            return 0;  							// Esci dalla funzione per evitare stallo
        }
    }
	
    serialInUse = true;							// Prendi il lock
	
    // Controllo se c'è spazio disponibile per scrivere sulla porta seriale
    if (serialPort->availableForWrite() >= length)
    {
        for (size_t i = 0; i < length; i++)
        {
            serialPort->write(data[i]);
        }

        // Debug: stampa i dati inviati sulla seriale di debug
        // debugSerial.print("Dati inviati: ");
        // for (size_t i = 0; i < length; i++)
        // {
        // 	  debugSerial.printf("0x%02X ", data[i]);
        // }
        // debugSerial.println();
    }
    else
    {
        // Debug: Errore nel caso in cui non ci sia spazio sufficiente
        debugSerial.println("Errore: spazio insufficiente sulla seriale per inviare i dati.");
    }
	serialInUse = false;						// Rilascia il lock
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

uint16_t IcomSim::isChanged()
{
    uint8_t changedFlags = 0; // Inizializza a 0, cioè nessun flag attivo

    if (Flags.frequencyChanged) 
	{
        changedFlags |= FLAG_FREQUENCY_CHANGED; // Imposta il bit corrispondente
        Flags.frequencyChanged = false;         // Resetta il flag
    }

    if (Flags.modeChanged) 
	{
        changedFlags |= FLAG_MODE_CHANGED; // Imposta il bit corrispondente
        Flags.modeChanged = false;         // Resetta il flag
    }

    if (Flags.sqlChanged) 
	{
        changedFlags |= FLAG_SQL_CHANGED; // Imposta il bit corrispondente
        Flags.sqlChanged = false;         // Resetta il flag
    }

    if (Flags.gainChanged) 
	{
        changedFlags |= FLAG_GAIN_CHANGED; // Imposta il bit corrispondente
        Flags.gainChanged = false;         // Resetta il flag
    }

    return changedFlags; // Restituisci tutti i flag attivi (0 se nessuno è attivo)
}


// ******************************************************************************************************************************
//
// ******************************************************************************************************************************
uint8_t IcomSim::getMode()
{
	return VfoData.Mode;
}

uint8_t IcomSim::getSquelch()
{
	return VfoData.Sql;
}

uint8_t IcomSim::getGain()
{
	return VfoData.Gain;
}

uint32_t IcomSim::getFrequency()
{
	return VfoData.Frequency;
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