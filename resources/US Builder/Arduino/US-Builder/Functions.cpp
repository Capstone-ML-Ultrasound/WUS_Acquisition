#include <SPI.h>
#include "Variables_Functions.h"
#include "SPI_US.h"

extern byte buffer[10];
extern byte versionFW;
byte BlueTooth;

void MANAGE_SPI()
{

  byte tab[12];
  byte tab1[512];

  if (Serial.available()) // --------------- USB
    {
      digitalWrite(mabt,1);
      BlueTooth=0;
      for (int i=0;i<12;i++)
        {
          tab[i]=Serial.read();
        }
      if (tab [0] ==140 and tab [1] ==140 and tab [2] ==140 and tab [3] ==140 and tab [4] ==140)
       {
        if (tab[5]==1)                          //ecriture
            {
              digitalWrite(E_MOSI,0);
              for (int i=0;i<tab[6];i++)
              {
              SPI.transfer(tab[i+7]);
              }
              digitalWrite(E_MOSI,1);
             }
        if (tab[5]==0)                        //lecture
            {
              int quotient,reste,SizeBuf;
              int nbpts= tab[6]*256+tab[7];
              SizeBuf=512;
              reste=nbpts%SizeBuf;
              quotient=(nbpts-reste)/SizeBuf;
            // lecture
              digitalWrite(E_MISO,0);
                if (quotient>0) {ReadSamples(quotient,SizeBuf,0);}  // send quotient*SizeBuf array (faster than sending byte/byte)
                if (reste>0) {ReadSamples(1,reste,0);}              // send 1*reste array (faster than sending byte/byte)
              digitalWrite(E_MISO,1);
              }
        if (tab[5]==2)  
            {
              //int nbpts= tab[6]*256+tab[7];
              //for (int i=0;i<nbpts;i++)
              //{
                Serial.write(analogRead(analogPin)/4);
              //}
            } 
        if (tab[5]==3)  
            {
              //int nbpts= tab[6]*256+tab[7];
              //for (int i=0;i<nbpts;i++)
              //{
                Serial.write(versionFW);
              //}
            } 
        }
        
      else
        {
          if (tab[5]==0) //lecture
            {
            int nbpts= tab[6]*256+tab[7];
            byte verif = 50;
            digitalWrite(E_MISO,0);
            for (int i=0;i<nbpts;i++)
              {
                Serial.write(verif); 
              }
            digitalWrite(E_MISO,1);
            }       
        }        
      }   




 if (Serial1.available()>=12) // --------- BT
    {
      BlueTooth=1;
      for (int i=0;i<12;i++)
        {
          tab[i]=Serial1.read();
        }
      if (tab [0] ==140 and tab [1] ==140 and tab [2] ==140 and tab [3] ==140 and tab [4] ==140)
        {
          if (tab[5]==1) //ecriture
            {
              digitalWrite(E_MOSI,0);
              for (int i=0;i<tab[6];i++)
                {
                  SPI.transfer(tab[i+7]);
                }
              digitalWrite(E_MOSI,1);
            }

          if (tab[5]==0) //lecture
            {
              int quotient,reste,SizeBuf;
              int nbpts= tab[6]*256+tab[7];
              SizeBuf=512;
              reste=nbpts%SizeBuf;
              quotient=(nbpts-reste)/SizeBuf;
              // lecture
              
              digitalWrite(E_MISO,0);
                if (quotient>0) {ReadSamples(quotient,SizeBuf,0);}  // send quotient*SizeBuf array (faster than sending byte/byte)
                if (reste>0) {ReadSamples(1,reste,0);}              // send 1*reste array (faster than sending byte/byte)
              digitalWrite(E_MISO,1);

            //   digitalWrite(E_MISO,0);
            //   for (int i=0;i<nbpts;i++)
            //     {
            //       tab1[i]= SPI.transfer(0);
            //     }
            //   digitalWrite(E_MISO,1);
            //   tab1[511] = analogRead(analogPin)/4;
            //   for (int i=0;i<nbpts;i++)
            //     {
            //       Serial1.write(tab1[i]);
            //     }
            }
          if (tab[5]==2)  
            {
              //int nbpts= tab[6]*256+tab[7];
              //for (int i=0;i<nbpts;i++)
              //{
                Serial1.write(analogRead(analogPin)/4);
              //}
            }
          if (tab[5]==3)  
            {
              //int nbpts= tab[6]*256+tab[7];
              //for (int i=0;i<nbpts;i++)
              //{
                Serial1.write(versionFW);
              //}
            }
        }
      else
        {
          if (tab[5]==0) //lecture
            {
              int nbpts= tab[6]*256+tab[7];
              byte verif = 50;
              digitalWrite(E_MISO,0);
              for (int i=0;i<nbpts;i++)
                {
                  Serial1.write(verif); 
                }
              digitalWrite(E_MISO,1);
            }       
        }            
    }   
}
