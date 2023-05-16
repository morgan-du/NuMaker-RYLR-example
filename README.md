# RYLR998 on NuMaker IoT boards example

RYLR998 is a transceiver module features the LoRa long range modem that provides long range communication. It provides AT Command way to simplify module control.

This example uses the ATCmdParser API to construct the driver class to control the RYLR998 module on the Nuvoton NuMaker IoT boards. It also provides a simple send and receive operation.

## Import and Build
You can use Mbed OS tool to import this example, specify the NuMaker-IoT boards, then build it.
To build send example, set BUILD_TX in main.cpp to 1.
To build receive example, set BUILD_TX in main.cpp to 0.

## Module Connection
The example assumes that the NuMaker IoT board and RYLR998 module are connected in the following way.

NuMaker IoT Board        Reyax RYLR998
       VCC <------------> VDD  
Arduino D2 (GPIO) <-----> NRST
Arduino D1 (TX) <-------> RXD
Arduino D0 (RX) <-------> TXD
       GND <------------> GND 

