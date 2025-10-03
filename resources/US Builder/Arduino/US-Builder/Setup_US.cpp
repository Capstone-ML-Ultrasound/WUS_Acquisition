#include <SPI.h>
#include "Variables_Functions.h"
#include "SPI_US.h"


void Setup_US()
{
  
  SEND_GAIN_dB(40.0);                           // Gain=40dB
  SEND_COMPRESS_bin(0);                         // 0=> Compressor OFF
  SEND_DELAY_us(0);                             // Delay=0us
  SEND_VOLTAGE_Volt(50.0);                      // Volt=50
  SEND_WIDTH_ns(55.5);                          // =SEND_WIDTH_bin(6)
  SEND_PRF_kHz(0.1);                            // PRF=100Hz
  SEND_MODE(0);                                 // MODE=Pulse Echo
  SEND_SCALE_us(51.0);                          // Scale=51us
  SEND_FILTER_bin(BROADBAND);                   // Filter=BroadBand=No filter
  SEND_SAMPLINGFREQ_bin(F_80MHz);               // Sampling frequency = 80MHz => resolution=12.5ns

  SEND_ECHOSTARTPOSWIDTH_us(0,3.5);             // Echostart => Pos=0us / Width=3.5us
  SEND_ECHOSTARTPOLARITY_pourcent(50);          // Echostart => Polarity +50%
  
  SEND_GATE1POSWIDTH_us(7.1,3.5);               // Gate1 => Pos=7.1us / Width=3.5us
  SEND_GATE2POSWIDTH_us(15.1,3.5);              // Gate2 => Pos=15.1us / Width=3.5us
  SEND_GATE3POSWIDTH_us(23.5,3.5);              // Gate3 => Pos=23.5us / Width=3.5us
  SEND_GATE1THRESHOLDALFILTER_pourcent(0,80.7); // Gate1 => AlFilter[7..0]=0 ! / Threshold=80.7%
  SEND_GATE2THRESHOLDALFILTER_pourcent(0,60.7); // Gate2 => AlFilter[7..0]=0 ! / Threshold=60.7%
  SEND_GATE3THRESHOLDALFILTER_pourcent(0,40.7); // Gate3 => AlFilter[7..0]=0 ! / Threshold=40.7%
  SEND_ANALOGSETPOLARITY_bin(ANALOG_TOTAL,ANALOG_TOTAL,ANALOG_TOTAL,BOTH_WAVES);    // ANALOG1 = OFF / ANALOG2 = OFF / ANALOG3 = OFF / Polarity = Both waves !!! All gates control the same polarity !!!
  SEND_ALDELAY_bin(0,0);             // Duration = 0 / All Alarms = 0 => On Appears

  SEND_NBSAMPLES_bin(4090);         // Only used for AUTOMATIC A-scan storage => Unused, here
  SEND_ONOFFDAC(1);                 // DAC OFF - Mode Read
  SEND_READPORT(ASCAN_8bit);
 
  
}
