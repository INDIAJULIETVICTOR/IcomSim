# IcomSim Arduino Library

## Description
This is an Arduino library for simulating the **Icom CI-V protocol**. The library allows the Arduino board to emulate an Icom-compatible radio, providing a way to integrate with CI-V based control systems and communicate with existing radio equipment.

## Features
- Simulates the Icom CI-V protocol for Arduino.
- Enables the Arduino to be recognized as an Icom radio by other CI-V devices.
- Easy integration with Arduino projects.
- Example sketches provided to help you get started quickly.

## Installation
To install the library:

1. Download the repository as a ZIP file.
2. Open the Arduino IDE.
3. Go to **Sketch** > **Include Library** > **Add .ZIP Library...**.
4. Select the downloaded ZIP file to add the library.

## Getting Started
### Basic Usage
Include the library in your sketch:

```cpp
#include <IcomSim.h>

IcomSim icom;   // Create an instance of IcomSim

void setup() 
{
    Serial.begin(9600);     // Initialize serial communication
    IcomSim radio(Serial);  // Initialize IcomSim for communication
}

void loop() 
{
  radio.processCIVCommand();

  switch(radio.isChanged())
  {
    .....
  }
}
```

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author
- **Fabrizio Palumbo (IU0IJV)**
- Contact: [t.me/IU0IJV](https://t.me/IU0IJV)

## Contributions
Contributions are welcome! Please feel free to submit pull requests to improve the library or fix issues.

## Issues
If you encounter any issues or have suggestions for improvements, please open an issue on GitHub.
