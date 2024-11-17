
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
#include <ArduinoQueue.h> 


#define RX_PIN A3                                            // pin usati da softwareserial
#define TX_PIN A4
#define QUEUE_MAX_SIZE 10  // Dimensione massima della coda


uint8_t vfonum=0;

SoftwareSerial debugSerial(RX_PIN, TX_PIN);

      // MODE_AM = 0x00   # Codice per AM
      // MODE_FM = 0x01   # Codice per FM
      // MODE_SSB = 0x02  # Codice per SSB

// Struttura per memorizzare i messaggi da inviare
struct SerialMessage 
{
    uint8_t data[32];
    size_t length;
};

// Inizializza la coda con dimensione massima
ArduinoQueue<SerialMessage> serialQueue(QUEUE_MAX_SIZE);


// ******************************************************************************************************************************
//
// ******************************************************************************************************************************

// Costruttore della classe IcomSim
IcomSim::IcomSim(Stream& serial)
{
    serialPort = &serial;
}


// ******************************************************************************************************************************
// Funzione di inizializzazione che accetta una struttura di dati iniziale
// ******************************************************************************************************************************
bool IcomSim::Initialize(VfoData_t* initData1, VfoData_t* initData2)
{
	// Inizializza la seriale per il debug
    debugSerial.begin(9600);

    if (initData1 == nullptr || initData2 == nullptr) {
        debugSerial.println("Puntatori errati");
        return false;
    }

    // Assegna i puntatori ai membri della classe
    VfoData[0] = initData1;
    VfoData[1] = initData2;

    debugSerial.println("Debug Serial Attivata");

    // Resetta tutti i flag
    memset(&Flags, 0, sizeof(Flags_t));
	return true;
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
						// ---------------------------------------------------- FREQUENCY
                        case COMMAND_SET_FREQUENCY:
                            if (dataLength >= 5)
                            {
                                uint32_t frequency = 0;

                                // Decodifica i 6 byte BCD ricevuti, invertendo l'ordine dei byte e i nibble
                                for (int i = 5; i >= 0; i--)
                                {
                                    // Inverti l'ordine dei nibble in ciascun byte
                                    uint8_t inverted_byte = (data[i] << 4) | (data[i] >> 4);

                                    uint8_t high_nibble = (inverted_byte >> 4) & 0x0F;  // Prendi la cifra più significativa (ora invertita)
                                    uint8_t low_nibble = inverted_byte & 0x0F;           // Prendi la cifra meno significativa (ora invertita)

                                    frequency = (frequency * 100) + (high_nibble * 10) + low_nibble;
                                }

                                VfoData[vfonum]->Frequency = frequency;
                                Flags.frequencyChanged = true;

                                // Debug: stampa la frequenza decodificata
                                // Debug_Print("Frequenza decodificata (Hz): %ld\n\r", currentFrequency);
                            }
                            break;
	
						case COMMAND_GET_FREQUENCY:
							send_frequency(VfoData[vfonum]->Frequency,addressFrom, addressTo);
							break;

						// ---------------------------------------------------- SQUELCH
                        case COMMAND_SET_SQUELCH:
                            if (dataLength > 0)
                            {
                                VfoData[vfonum]->Sql = data[0];
                                Flags.sqlChanged = true;
                            }
                            break;
							
						case COMMAND_GET_SQUELCH:
							send_command(command, VfoData[vfonum]->Sql, addressFrom, addressTo);
							break;	

						// ---------------------------------------------------- MODE
                        case COMMAND_SET_MODE:
                            if (dataLength > 0)
                            {
                               VfoData[vfonum]->Mode = data[0];
                               Flags.modeChanged = true;
                            }
                            break;							

						// ---------------------------------------------------- RFGAIN
                        case COMMAND_SET_RFGAIN:
                            if (dataLength > 0)
                            {
                                VfoData[vfonum]->Gain = data[0];
                                Flags.gainChanged = true;
                            }
                            break;
							
						case COMMAND_GET_RFGAIN:
							send_command(command, VfoData[vfonum]->Gain, addressFrom, addressTo);
							break;
						
						// ---------------------------------------------------- MONITOR
						case COMMAND_SET_MONITOR:
							Flags.monitorChanged = true;	
							break;
							
						// ---------------------------------------------------- BANDWITH
						case COMMAND_SET_BANDWIDTH:
							if (dataLength > 0)
                            {
                                VfoData[vfonum]->bw = data[0];
                                Flags.bwChanged = true;
                            }
                            break;
						
						
						case COMMAND_GET_BANDWIDTH:
							send_command(command, VfoData[vfonum]->bw, addressFrom, addressTo);
							break;
							
						// ---------------------------------------------------- TXPOWER
						case COMMAND_SET_TX_POWER:
							if (dataLength > 0)
                            {
                                VfoData[vfonum]->txp = data[0];
                                Flags.txpChanged = true;
                            }
                            break;
						
						
						case COMMAND_GET_TX_POWER:
							send_command(command, VfoData[vfonum]->txp, addressFrom, addressTo);
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

uint16_t IcomSim::isChanged()
{
    uint8_t changedFlags = 0; 											// Inizializza a 0, cioè nessun flag attivo

    if (Flags.frequencyChanged) changedFlags |= FLAG_FREQUENCY_CHANGED; 
    if (Flags.modeChanged)    	changedFlags |= FLAG_MODE_CHANGED; 		
    if (Flags.sqlChanged)     	changedFlags |= FLAG_SQL_CHANGED; 		
    if (Flags.gainChanged)    	changedFlags |= FLAG_GAIN_CHANGED; 		
	if (Flags.modeChanged)    	changedFlags |= FLAG_MODE_CHANGED; 		
    if (Flags.sqlChanged)     	changedFlags |= FLAG_SQL_CHANGED; 		
    if (Flags.monitorChanged) 	changedFlags |= FLAG_MONITOR_CHANGED; 	
    if (Flags.sqlChanged)     	changedFlags |= FLAG_SQL_CHANGED; 		
	if (Flags.bwChanged)     	changedFlags |= FLAG_BW_CHANGED; 		
	if (Flags.txpChanged)     	changedFlags |= FLAG_TXP_CHANGED; 		
	
	memset(&Flags, 0, sizeof(Flags_t));

    return changedFlags; // Restituisci tutti i flag attivi (0 se nessuno è attivo)
}

// ******************************************************************************************************************************
//
// ******************************************************************************************************************************
void IcomSim::send_frequency(uint32_t frequency, uint8_t addressFrom, uint8_t addressTo)
{
    uint8_t message[16];
    message[0] = 0xFE;
    message[1] = 0xFE;
    message[2] = addressFrom;
    message[3] = addressTo;
    message[4] = COMMAND_GET_FREQUENCY;  			// Comando di risposta per GET_FREQUENCY

    // Converti la frequenza in formato BCD (Binary Coded Decimal)
    for (int i = 5; i >= 0; i--) 
    {
        message[5+i] = (frequency % 10) | ((frequency / 10 % 10) << 4);
        frequency /= 100;
    }

    message[11] = 0xFD;  							// Byte di fine messaggio

    sendToSerial(message, sizeof(message));			// Invia il messaggio usando la funzione centralizzata
}

// ******************************************************************************************************************************
//
// ******************************************************************************************************************************
void IcomSim::send_rssi(uint16_t value, uint8_t addressFrom, uint8_t addressTo)
{
    uint8_t message[8];
    message[0] = 0xFE;
    message[1] = 0xFE;
    message[2] = addressFrom;
    message[3] = addressTo;
    message[4] = COMMAND_GET_RSSI;  				// Comando di risposta
													// Dividi il valore RSSI in due byte (LSB e MSB)
    message[5] = value & 0xFF;       				// Byte meno significativo
    message[6] = (value >> 8) & 0xFF; 				// Byte più significativo

    message[7] = 0xFD;  							// Byte di fine messaggio

    sendToSerial(message, sizeof(message));			// Invia il messaggio usando la funzione centralizzata
}

// ******************************************************************************************************************************
//
// ******************************************************************************************************************************
void IcomSim::send_status(uint8_t vfo, uint8_t addressFrom, uint8_t addressTo)
{
	uint16_t value = VfoData[vfo]->Flag.Flags;
    uint8_t message[8];
	
    message[0] = 0xFE;
    message[1] = 0xFE;
    message[2] = addressFrom;
    message[3] = addressTo;
    message[4] = COMMAND_GET_STATUS;  				// Comando di risposta
													// Dividi il valore RSSI in due byte (LSB e MSB)
    message[5] = value & 0xFF;       				// Byte meno significativo
    message[6] = (value >> 8) & 0xFF; 				// Byte più significativo

    message[7] = 0xFD;  							// Byte di fine messaggio

    sendToSerial(message, sizeof(message));			// Invia il messaggio usando la funzione centralizzata
}







// ******************************************************************************************************************************
//
// ******************************************************************************************************************************
void IcomSim::send_command(uint8_t command, uint8_t value, uint8_t addressFrom, uint8_t addressTo)
{
    uint8_t message[7];
    message[0] = 0xFE;                        // Byte di inizio messaggio
    message[1] = 0xFE;                        // Byte di inizio messaggio
    message[2] = addressFrom;                 // Indirizzo del mittente
    message[3] = addressTo;                   // Indirizzo del destinatario
    message[4] = command;                     // Comando specifico passato come argomento
    message[5] = value & 0xFF;                // Valore specifico passato come argomento
    message[6] = 0xFD;                        // Byte di fine messaggio

    sendToSerial(message, sizeof(message));   // Invia il messaggio usando la funzione centralizzata
}

// ******************************************************************************************************************************
//
// ******************************************************************************************************************************

void IcomSim::sendToSerial(const uint8_t* data, size_t length) 
{
    if (serialQueue.isFull()) 
	{
        debugSerial.println("Errore: coda di trasmissione piena");
        return;
    }

    SerialMessage msg;
    memcpy(msg.data, data, length);
    msg.length = length;
    serialQueue.enqueue(msg);
}

// Elabora la coda nel loop principale
void IcomSim::processSerialQueue() 
{
    if (!serialQueue.isEmpty()) 
	{
        SerialMessage msg = serialQueue.dequeue();

        if (serialPort->availableForWrite() >= msg.length) 
		{
            serialPort->write(msg.data, msg.length);
            // serialPort->flush();
        } 
		else 
		{
            debugSerial.println("Errore: spazio seriale insufficiente per il messaggio");
        }
    }
}

/* void IcomSim::sendToSerial(const uint8_t* data, size_t length)
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
 */



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
void IcomSim::Debug_Print(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char buffer[128]; // Buffer per formattare la stringa
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    debugSerial.print(buffer); // Stampa sulla seriale di debug
}