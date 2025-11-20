#include <SPI.h>
#include "Variables_Functions.h"

extern byte buffer[10];
extern byte BlueTooth;


void SEND_BYTES(byte Len)
{
  digitalWrite(E_MOSI,0); // 0=Disable => See SPI_Mode1 => Enable_MOSI is active at 0 level => beginning of writing
  for (int i=0;i<Len;i++)
  {
    SPI.transfer(buffer[(Len-(i+1))]);  // The last buffer element is sent in 1st => buffer is sent from bottom to top
  }
  digitalWrite(E_MOSI,1); // 1=Disable => See SPI_Mode1 => Enable_MOSI is active at 0 level => end of writing
}

void ReadSamples(int passe,int Nb, byte type)
{
  int i,j;
  byte tab[Nb];
  
  for (i=0;i<passe;i++)
  {
///////////// A-scan //////////////////////////   
    if (type==0)
    {
      for (j=0;j<Nb;j++)
      {
        tab[j]=SPI.transfer(0);
      }
    }
    else
/////////////////// SawCurve for debugging ////////
    {
      for (j=0;j<Nb;j++)
      {
        tab[j]=j%256;
      }
    }
    if (BlueTooth==1)
      {
        Serial1.write(&tab[0],Nb);      // Tansfering several(X) arrays of 1024 bytes is faster than 1 array of X*1024 bytes
      }
    else
      {
        Serial.write(&tab[0],Nb);      // Tansfering several(X) arrays of 1024 bytes is faster than 1 array of X*1024 bytes
      }
  }
}

// I added those -Leo

void SEND_SAMPLING_REQ_bin()
{
  digitalWrite(E_MOSI, 0); 
  
  SPI.transfer(ADDR_Start);
  
  digitalWrite(E_MOSI, 1);

}

void SEND_AUTO_SAMPLING_REQ_bin(unsigned int samples_bin)
{
  byte samples_lsb, samples_msb;
  samples_lsb = samples_bin & 0xFF;
  samples_msb = (samples_bin >> 8) & 0xFF;

  digitalWrite(E_MOSI, 0);
  
  SPI.transfer(ADDR_NbSamples);
  SPI.transfer(samples_msb);
  SPI.transfer(samples_lsb);
  
  digitalWrite(E_MOSI, 1);
}


void SEND_GAIN_bin(unsigned int Gain_bin)
{
  byte Gain_LSB,Gain_MSB;
  
  Gain_LSB=Gain_bin%256;
  Gain_MSB=(Gain_bin-Gain_LSB)/256;
  //Serial.print(String(Gain_dB)+"="+String(Gain_bin)+"="+String(Gain_LSB)+"="+String(Gain_MSB)+" / ");
  
  digitalWrite(E_MOSI,0);
  SPI.transfer(0);
  SPI.transfer(Gain_MSB);
  SPI.transfer(Gain_LSB);
  SPI.transfer(ADDR_Gain); 
  digitalWrite(E_MOSI,1);
}


void SEND_GAIN_dB(double Gain_dB)
{
   unsigned int Gain_bin;

  Gain_bin=(10.108*Gain_dB)+65;
  SEND_GAIN_bin(Gain_bin);
}

void SEND_COMPRESS_bin(byte Compress_bin)
{
  digitalWrite(E_MOSI,0);
  SPI.transfer(Compress_bin);
  SPI.transfer(ADDR_Compres); 
  digitalWrite(E_MOSI,1);
}

void SEND_DELAY_bin(unsigned int Delay_bin)
{
  byte Delay_LSB,Delay_MSB;

  Delay_LSB=Delay_bin%256;
  Delay_MSB=(Delay_bin-Delay_LSB)/256;
  
  digitalWrite(E_MOSI,0);
  SPI.transfer(Delay_MSB);
  SPI.transfer(Delay_LSB);
  SPI.transfer(ADDR_Delay); 
  digitalWrite(E_MOSI,1);
}  

void SEND_DELAY_us(double Delay_us)
{
   unsigned int Delay_bin;

  Delay_bin=Delay_us/0.025;
  SEND_DELAY_bin(Delay_bin);
}

void SEND_VOLTAGE_bin(byte Voltage_bin)
{
  digitalWrite(E_MOSI,0);
  SPI.transfer(Voltage_bin);
  SPI.transfer(ADDR_Voltage); 
  digitalWrite(E_MOSI,1);
}

void SEND_VOLTAGE_Volt(double Voltage_V)
{
  byte Voltage_bin;

  Voltage_bin=(98*Voltage_V/180)+81.777;
  SEND_VOLTAGE_bin(Voltage_bin);
}

void SEND_WIDTH_bin(byte Width_bin)
{
  digitalWrite(E_MOSI,0);
  SPI.transfer(Width_bin);
  SPI.transfer(ADDR_Width); 
  digitalWrite(E_MOSI,1);
}
void SEND_WIDTH_ns(double Width_ns)
{
  byte Width_bin;

  Width_bin=(Width_ns-18)/6.25;
  SEND_WIDTH_bin(Width_bin);
}


void SEND_PRF_bin(long PRF_bin)
{
  byte PRF_LSB,PRF_middle,PRF_MSB;
  unsigned int PRF_bLSB,PRF_bMSB;

  PRF_bLSB=PRF_bin%65536;
  PRF_bMSB=(PRF_bin-PRF_bLSB)/65536;
  PRF_LSB=PRF_bLSB%256;
  PRF_middle=(PRF_bLSB-PRF_LSB)/256;
  PRF_MSB=(PRF_bMSB)%256;
  
  digitalWrite(E_MOSI,0);
  SPI.transfer(PRF_MSB);
  SPI.transfer(PRF_middle);
  SPI.transfer(PRF_LSB);
  SPI.transfer(ADDR_Prf); 
  digitalWrite(E_MOSI,1);
}

void SEND_PRF_kHz(double PRF_kHz)
{
  long PRF_bin;

  PRF_bin=1/(PRF_kHz*1000*0.000000025);
  SEND_PRF_bin(PRF_bin);
}

void SEND_SCALE_bin(unsigned int Scale_bin)
{
  byte Scale_LSB,Scale_MSB;

  Scale_LSB=Scale_bin%256;
  Scale_MSB=(Scale_bin-Scale_LSB)/256;
  
  digitalWrite(E_MOSI,0);
  SPI.transfer(Scale_MSB);
  SPI.transfer(Scale_LSB);
  SPI.transfer(ADDR_Scale); 
  digitalWrite(E_MOSI,1);
}  

void SEND_SCALE_us(double Scale_us)
{
  unsigned int Scale_bin;

  Scale_bin=Scale_us/0.025;
  SEND_SCALE_bin(Scale_bin);
}

void SEND_FILTER_bin(byte Filter_bin)
{
  digitalWrite(E_MOSI,0);
  SPI.transfer(Filter_bin);
  SPI.transfer(ADDR_Filter); 
  digitalWrite(E_MOSI,1);
}

void SEND_SAMPLINGFREQ_bin(byte SamplingFreq_bin)
{
  digitalWrite(E_MOSI,0);
  SPI.transfer(SamplingFreq_bin);
  SPI.transfer(ADDR_SamplingFreq); 
  digitalWrite(E_MOSI,1);
  //Serial.print("address:"+String(ADDR_SamplingFreq)+"="+String(SamplingFreq_bin)+" / ");
}

void SEND_CHANNEL_bin(byte Tx, byte Rx)
{
  digitalWrite(E_MOSI,0);
  SPI.transfer(Rx);
  SPI.transfer(Tx);
  SPI.transfer(ADDR_ChannelTxRx); 
  digitalWrite(E_MOSI,1);
}

void SEND_ECHOSTARTPOSWIDTH_bin(unsigned int Pos_bin, unsigned int Width_bin)
{
  byte Pos_LSB,Pos_MSB,Width_LSB,Width_MSB;

  Pos_LSB=Pos_bin%256;
  Pos_MSB=(Pos_bin-Pos_LSB)/256;
  Width_LSB=Width_bin%256;
  Width_MSB=(Width_bin-Width_LSB)/256;
  
  digitalWrite(E_MOSI,0);
  SPI.transfer(Width_MSB);
  SPI.transfer(Width_LSB);
  SPI.transfer(Pos_MSB);
  SPI.transfer(Pos_LSB);
  SPI.transfer(ADDR_PosES); 
  digitalWrite(E_MOSI,1);
  //Serial.print("Pos_bin:"+String(Pos_bin)+"="+"Width_bin:"+String(Width_bin)+" / ");
  //Serial.print("address:"+String(ADDR_SamplingFreq)+"="+String(SamplingFreq_bin)+" / ");
}

void SEND_ECHOSTARTPOSWIDTH_us(double Pos_us, double Width_us)
{
   unsigned int Pos_bin, Width_bin;

   Pos_bin=Pos_us/0.025;
   Width_bin=Width_us/0.025;
   SEND_ECHOSTARTPOSWIDTH_bin(Pos_bin,Width_bin);
}

void SEND_ECHOSTARTPOLARITY_bin(byte Polarity_bin)
{
  digitalWrite(E_MOSI,0);
  SPI.transfer(Polarity_bin);
  SPI.transfer(ADDR_PolarES); 
  digitalWrite(E_MOSI,1);
}

void SEND_ECHOSTARTPOLARITY_pourcent(double Polarity_pourcent)    // A vérifier avec SB !!!!!
{
  byte Polarity_bin;

  Polarity_bin=1.27*Polarity_pourcent+128;
  //Serial.print(String(Polarity_pourcent)+"="+String(Polarity_bin)+" / ");
  SEND_ECHOSTARTPOLARITY_bin(Polarity_bin);
}

void SEND_GATE1POSWIDTH_bin(unsigned int Pos_bin, unsigned int Width_bin)
{
  byte Pos_LSB,Pos_MSB,Width_LSB,Width_MSB;

  Pos_LSB=Pos_bin%256;
  Pos_MSB=(Pos_bin-Pos_LSB)/256;
  Width_LSB=Width_bin%256;
  Width_MSB=(Width_bin-Width_LSB)/256;
  
  digitalWrite(E_MOSI,0);
  SPI.transfer(Width_MSB);
  SPI.transfer(Width_LSB);
  SPI.transfer(Pos_MSB);
  SPI.transfer(Pos_LSB);
  SPI.transfer(ADDR_Gate1PosWidth); 
  digitalWrite(E_MOSI,1);
  //Serial.print("Pos_bin:"+String(Pos_bin)+"="+"Width_bin:"+String(Width_bin)+" / ");
  //Serial.print("address:"+String(ADDR_SamplingFreq)+"="+String(SamplingFreq_bin)+" / ");
}

void SEND_GATE1POSWIDTH_us(double Pos_us, double Width_us)
{
   unsigned int Pos_bin, Width_bin;

   Pos_bin=Pos_us/0.025;
   Width_bin=Width_us/0.025;
   SEND_GATE1POSWIDTH_bin(Pos_bin,Width_bin);
}

void SEND_GATE2POSWIDTH_bin(unsigned int Pos_bin, unsigned int Width_bin)
{
  byte Pos_LSB,Pos_MSB,Width_LSB,Width_MSB;

  Pos_LSB=Pos_bin%256;
  Pos_MSB=(Pos_bin-Pos_LSB)/256;
  Width_LSB=Width_bin%256;
  Width_MSB=(Width_bin-Width_LSB)/256;
  
  digitalWrite(E_MOSI,0);
  SPI.transfer(Width_MSB);
  SPI.transfer(Width_LSB);
  SPI.transfer(Pos_MSB);
  SPI.transfer(Pos_LSB);
  SPI.transfer(ADDR_Gate2PosWidth); 
  digitalWrite(E_MOSI,1);
  //Serial.print("Pos_bin:"+String(Pos_bin)+"="+"Width_bin:"+String(Width_bin)+" / ");
  //Serial.print("address:"+String(ADDR_SamplingFreq)+"="+String(SamplingFreq_bin)+" / ");
}

void SEND_GATE2POSWIDTH_us(double Pos_us, double Width_us)
{
   unsigned int Pos_bin, Width_bin;

   Pos_bin=Pos_us/0.025;
   Width_bin=Width_us/0.025;
   SEND_GATE2POSWIDTH_bin(Pos_bin,Width_bin);
}

void SEND_GATE3POSWIDTH_bin(unsigned int Pos_bin, unsigned int Width_bin)
{
  byte Pos_LSB,Pos_MSB,Width_LSB,Width_MSB;

  Pos_LSB=Pos_bin%256;
  Pos_MSB=(Pos_bin-Pos_LSB)/256;
  Width_LSB=Width_bin%256;
  Width_MSB=(Width_bin-Width_LSB)/256;
  
  digitalWrite(E_MOSI,0);
  SPI.transfer(Width_MSB);
  SPI.transfer(Width_LSB);
  SPI.transfer(Pos_MSB);
  SPI.transfer(Pos_LSB);
  SPI.transfer(ADDR_Gate3PosWidth); 
  digitalWrite(E_MOSI,1);
  //Serial.print("Pos_bin:"+String(Pos_bin)+"="+"Width_bin:"+String(Width_bin)+" / ");
  //Serial.print("address:"+String(ADDR_SamplingFreq)+"="+String(SamplingFreq_bin)+" / ");
}

void SEND_GATE3POSWIDTH_us(double Pos_us, double Width_us)
{
   unsigned int Pos_bin, Width_bin;

   Pos_bin=Pos_us/0.025;
   Width_bin=Width_us/0.025;
   SEND_GATE3POSWIDTH_bin(Pos_bin,Width_bin);
}

void SEND_GATE1THRESHOLDALFILTER_bin(byte AlFilter_bin, byte Threshold_bin)
{
  digitalWrite(E_MOSI,0);
  SPI.transfer(Threshold_bin);
  SPI.transfer(AlFilter_bin);
  SPI.transfer(ADDR_Gate1ThresholdAlFilter); 
  digitalWrite(E_MOSI,1);
}

void SEND_GATE1THRESHOLDALFILTER_pourcent(byte AlFilter, double Threshold_pourcent)
{
  byte Threshold_bin;

  Threshold_bin=Threshold_pourcent*2.55;
  //Serial.print(String(Threshold_pourcent)+"="+String(Threshold_bin)+" / ");
  SEND_GATE1THRESHOLDALFILTER_bin(AlFilter,Threshold_bin);
}

void SEND_GATE2THRESHOLDALFILTER_bin(byte AlFilter_bin, byte Threshold_bin)
{
  digitalWrite(E_MOSI,0);
  SPI.transfer(Threshold_bin);
  SPI.transfer(AlFilter_bin);
  SPI.transfer(ADDR_Gate2ThresholdAlFilter); 
  digitalWrite(E_MOSI,1);
}

void SEND_GATE2THRESHOLDALFILTER_pourcent(byte AlFilter, double Threshold_pourcent)
{
  byte Threshold_bin;

  Threshold_bin=Threshold_pourcent*2.55;
  //Serial.print(String(Threshold_pourcent)+"="+String(Threshold_bin)+" / ");
  SEND_GATE2THRESHOLDALFILTER_bin(AlFilter,Threshold_bin);
}

void SEND_GATE3THRESHOLDALFILTER_bin(byte AlFilter_bin, byte Threshold_bin)
{
  digitalWrite(E_MOSI,0);
  SPI.transfer(Threshold_bin);
  SPI.transfer(AlFilter_bin);
  SPI.transfer(ADDR_Gate3ThresholdAlFilter); 
  digitalWrite(E_MOSI,1);
}

void SEND_GATE3THRESHOLDALFILTER_pourcent(byte AlFilter, double Threshold_pourcent)
{
  byte Threshold_bin;

  Threshold_bin=Threshold_pourcent*2.55;
  //Serial.print(String(Threshold_pourcent)+"="+String(Threshold_bin)+" / ");
  SEND_GATE3THRESHOLDALFILTER_bin(AlFilter,Threshold_bin);
}

void SEND_ANALOGSETPOLARITY_bin(byte Analog1Set, byte Analog2Set, byte Analog3Set, byte Polar_bin)
{
  digitalWrite(E_MOSI,0);
  SPI.transfer(Polar_bin);
  SPI.transfer(Analog3Set);  
  SPI.transfer(Analog2Set);  
  SPI.transfer(Analog1Set);  
  SPI.transfer(ADDR_AnalogOutput); 
  digitalWrite(E_MOSI,1);
}

void SEND_NBSAMPLES_bin(unsigned int NbSamples)
{
  byte NbSamples_LSB,NbSamples_MSB;

  NbSamples_LSB=NbSamples%256;
  NbSamples_MSB=(NbSamples-NbSamples_LSB)/256;
  
  digitalWrite(E_MOSI,0);
  SPI.transfer(NbSamples_MSB);
  SPI.transfer(NbSamples_LSB);
  SPI.transfer(ADDR_NbSamples); 
  digitalWrite(E_MOSI,1);
}

void SEND_READPORT(byte ReadPort)
{
  digitalWrite(E_MOSI,0);
  SPI.transfer(ReadPort);
  SPI.transfer(ADDR_ReadPort); 
  digitalWrite(E_MOSI,1);
}

void SEND_ONOFFDAC(byte ONOFFDAC)    // 2bit!!! Bit0: 0=ON / 1=OFF!!!   Bit1: 0=Read / 1=Write
{
  digitalWrite(E_MOSI,0);
  SPI.transfer(ONOFFDAC);
  SPI.transfer(ADDR_ONOFFDAC); 
  digitalWrite(E_MOSI,1);
}

void SEND_MODE(byte MODE)    // Mode 0=Pulse Echo / 1 = pitch & catch
{
  digitalWrite(E_MOSI,0);
  SPI.transfer(MODE);
  SPI.transfer(ADDR_ModePE); 
  digitalWrite(E_MOSI,1);
}

void SEND_ALDELAY_bin(unsigned int Duration, byte AlarmSet)
{
  byte Duration_LSB,Duration_MSB;

  Duration_LSB=Duration%256;
  Duration_MSB=(Duration-Duration_LSB)/256;
  
  digitalWrite(E_MOSI,0);
  SPI.transfer(AlarmSet);
  SPI.transfer(Duration_MSB);
  SPI.transfer(Duration_LSB);
  SPI.transfer(ADDR_AlDelay); 
  digitalWrite(E_MOSI,1);
}
