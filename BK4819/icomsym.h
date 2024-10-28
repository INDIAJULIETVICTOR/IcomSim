#ifndef ICOMSIM_H
#define ICOMSIM_H

#include <Arduino.h>
#include <SoftwareSerial.h>

class IcomSim {
public:
    IcomSim(int rxPin, int txPin);
    void begin(long baudRate);
    void processCommand();
    void setFrequency(long frequency);
    long getFrequency();
    void setMode(String mode);
    String getMode();

private:
    SoftwareSerial* serialPort;
    long currentFrequency;
    String currentMode;

    void sendResponse(const String& response);
};

#endif
