High-Level Architecture

The acquisition system is divided into three main components:

- Higher Level PC Interface (C++/Python) 
- Arduino SoC (C++)
- US Board (Raw bytes)

The communication between the PC and the Arduino is over serial USB, and the communication between the Arduino & the US-key is through SPI protocol.














If error: 

Failed to open port \\.\COM4

In powershell,

Get-PnpDevice -Class Ports

Serial-USB device will probably indicate a different port simply swap to tht one.

// covariance matrix with AR
// ARIMA / ARMA 
//  