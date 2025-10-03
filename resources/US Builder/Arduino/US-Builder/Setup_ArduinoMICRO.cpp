#include <SPI.h>
#include "Variables_Functions.h"

void Setup_ArduinoMICRO()
{
Serial.begin(2000000);  // Serial at 2.000.000 bauds
pinMode(E_MOSI,OUTPUT); // Digital Port D2 used for Enable_MOSI
pinMode(E_MISO,OUTPUT); // Digital Port D3 used for Enable_MISO
pinMode(mabt,OUTPUT);   // Digital Port D5 used for Enable Bluetooth

digitalWrite(E_MOSI,1); // 1=Disable => See SPI_Mode1 => Enable_MOSI is active at 0 level
digitalWrite(E_MISO,1); // 1=Disable => See SPI_Mode1 => Enable_MISO is active at 0 level

digitalWrite(mabt,0);   // Bluetooth mode

SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));   // SCLK=1MHz ///// MSBFIRST or LSBFIRST ////// SPI_MODE0, SPI_MODE1, SPI_MODE2, or SPI_MODE3
SPI.begin();
Serial1.begin(115200);  // Serial at 115.200 bauds for BT
}
