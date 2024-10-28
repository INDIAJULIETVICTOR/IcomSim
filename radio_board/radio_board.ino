/*
 * Project Name: Radio Firmware
 * File: radio_manager.c
 *
 * Copyright (C) 2024 Fabrizio Palumbo (IU0IJV)
 * 
 * This program is distributed under the terms of the MIT license.
 * You can obtain a copy of the license at:
 * https://opensource.org/licenses/MIT
 *
 * DESCRIPTION:
 * Implementation of the control system for the experimental radio module based on the Beken BK4819.
 *
 * AUTHOR: Fabrizio Palumbo
 * CREATION DATE: October 27, 2024
 *
 * CONTACT: t.me/IU0IJV
 *
 * NOTES:
 * - This implementation requires the following libraries:
 *   - BK4819
 *   - IcomSYM
 *   - TaskScheduler
 * 
 * - Verify correct SPI pin configuration before use.
 */


#include <BK4819.h>
#include <TaskScheduler.h>
#include <IcomSim.h>

// #include <SoftwareSerial.h>

// #define RX_PIN 10
// #define TX_PIN 11

//--------------------------------------------------------- Creazione dell'oggetto Scheduler
Scheduler runner;

//--------------------------------------------------------- Funzioni dei task
void keyboard();
void sercomm();
void interrupt();
void smeter();

//--------------------------------------------------------- Definizione dei task
Task irq  (1, TASK_FOREVER, &interrupt);
Task keyb (10, TASK_FOREVER, &keyboard);                         // task che gestisce una pulsantiera
Task serc (10, TASK_FOREVER, &sercomm);
Task rssi (500, TASK_FOREVER, &smeter);

//--------------------------------------------------------- Definizione del chip beken
BK4819 beken(10, 11, 12, 13);                                   // Passa i pin CS, MOSI, MISO, e SCK

IcomSim radio(Serial);                                          // usa la seriale di sistema USB

// SoftwareSerial mySerial(RX_PIN, TX_PIN);
// IcomSim radio(mySerial);                                     // Usa SoftwareSerial per la comunicazione seriale

//--------------------------------------------------------- Definizione variabili
uint32_t frequenza = 74025UL * 100;  

bool mute = true;
bool monitor = false;
bool toggleState = false;   
bool lastButtonState = HIGH;  

//=============================================================================================
//
//=============================================================================================
void setup() 
{
    Serial.begin(19200); 
    // mySerial.begin(9600);                                    // Inizializza SoftwareSerial

    pinMode(9, INPUT_PULLUP);
    pinMode(8, INPUT_PULLUP);

    pinMode(7, OUTPUT);
    pinMode(6, OUTPUT);

    digitalWrite(6, LOW);
    digitalWrite(7, HIGH);

    beken.BK4819_Init();                                        // Inizializza il dispositivo BK4819

    beken.BK4819_Set_Frequency ( frequenza );                   // imposta frequenza
    beken.BK4819_Set_Filter_Bandwidth(BK4819_FILTER_BW_10k);	  // imposta BW e filtri audio
    beken.BK4819_RF_Set_Agc(0);
    beken.BK4819_Set_AGC_Gain(AGC_MAN, 20);                     // imposta AGC

    beken.BK4819_Set_AF(AF_FM);		                              // attiva demodulazione FM

    beken.BK4819_Squelch_Mode (RSSI);                           // tipo squelch

    beken.BK4819_Set_Squelch (104,107, 127,127, 127, 127);      // setup Squelch

    beken.BK4819_IRQ_Set ( Squelch_Lost | Squelch_Found );      // definizione interrupt

    beken.BK4819_RX_TurnOn();	                                  // accende modulo radio


  //--------------------------------------------------------- Aggiunta dei task allo scheduler
  runner.addTask(keyb);
  runner.addTask(serc);
  runner.addTask(irq);
  runner.addTask(rssi);

   //--------------------------------------------------------- Avvio dei task
  keyb.enable();
  serc.enable();
  irq.enable();
  //rssi.enable();
}


void mute_audio ( bool stato )
{
  if (stato)
  {
    digitalWrite(7, HIGH); 
    digitalWrite(6, LOW); 
  }
  else
  {
    digitalWrite(7, LOW); 
    digitalWrite(6, HIGH); 
  }
  mute = stato;

}



//=============================================================================================
//
//=============================================================================================
void loop() 
{
  runner.execute();
}

//=============================================================================================
//
//=============================================================================================
void interrupt ( void )
{
  if ( digitalRead(9) )
  {
    const uint16_t irqtype = beken.BK4819_Check_Irq_type();
    if(!monitor)
    {
      if (irqtype & Squelch_Lost) mute_audio(true);
      if (irqtype & Squelch_Found) mute_audio(false);
    }  

    // Serial.print("IRQ: ");
    // Serial.println(irqtype);

    beken.BK4819_Clear_Interrupt();
  }
}

//=============================================================================================
//
//=============================================================================================
void keyboard ( void )
{
   bool currentButtonState = digitalRead(8);
  
  //--------------------------------------------------------- Controlla se il pin 8 è passato da HIGH a LOW
  if (currentButtonState == LOW && lastButtonState == HIGH) 
  {
      monitor = mute;
      mute_audio(!mute);
      
      //--------------------------------------------------------- Piccola pausa per evitare rimbalzi
      delay(20);
  }

  //--------------------------------------------------------- Aggiorna lo stato precedente del pulsante
  lastButtonState = currentButtonState;
}


//============================================================================================
//
//=============================================================================================
void sercomm( void )
{
  radio.processCIVCommand();

  switch( radio.isChanged())
  {
    case COMMAND_SET_FREQUENCY:
      {
        uint32_t frequenza = radio.getFrequency()/100;

        digitalWrite(7, HIGH); 
        beken.BK4819_Set_Frequency(frequenza);
        digitalWrite(7, mute); 
      }
      break;

    case COMMAND_SET_SQUELCH:
      {
        uint8_t value = radio.getSquelch();
        beken.BK4819_Set_Squelch(value, value+4, 0,0,0,0);
        // beken.BK4819_RX_TurnOn();
      }
      break;  

    case COMMAND_SET_MODE:
      {
        // MODE_AM = 0x00   # Codice per AM
        // MODE_FM = 0x01   # Codice per FM
        // MODE_SSB = 0x02  # Codice per SSB

        uint8_t mode = radio.getMode();
        switch(mode)
        {
          case 0:   // # Codice per AM
            beken.BK4819_Set_AF(AF_AM);
            break;

          case 1:   // # Codice per AM
            beken.BK4819_Set_AF(AF_FM);
            break;

          case 2:   // # Codice per AM
            beken.BK4819_Set_AF(AF_DSB);
            break;  
        }
      }
      break;  
  }
}


//=============================================================================================
//
//=============================================================================================
void smeter( void )
{
  int16_t rssi = beken.BK4819_Get_RSSI();
  //uint16_t rssi = beken.BK4819_Read_Register(0x67) & 0x01FF;

  Serial.print("Frequenza: ");
  Serial.print((uint32_t)frequenza);
  Serial.print("   RSSI: ");
  Serial.print((int16_t)rssi);
  Serial.print("\r\n");

}

