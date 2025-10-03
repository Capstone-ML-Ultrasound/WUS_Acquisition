#include <SPI.h>
#include "Variables_Functions.h"
#include "Functions.h"
#include "Setup_ArduinoMICRO.h"
#include "Setup_US.h"
#include "SPI_US.h"

byte versionFW=1;


void setup() 
  {
    Setup_ArduinoMICRO();
    delay(1000);
    Setup_US();
  }


  
void loop()
  {  
    MANAGE_SPI();
  }
