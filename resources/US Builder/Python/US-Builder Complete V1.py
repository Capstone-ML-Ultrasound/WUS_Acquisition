from pylab import *
from serial import *
from ctypes import *
from matplotlib import pyplot as plt
import ctypes
import time
import pyqtgraph as pg
import numpy as np

Port="COM4"
Header=[140,140,140,140,140]        # Lenght=5
Write=ushort(1)
Read=ushort(0)
#                  [R/W , NbByte for SPI, 5 bytes for SPI part
SerialFrame=Header+[ 0  ,       0       , 0,0,0,0,0]  # SerialFrame[5]=R/W [6]=NbByte [7/8/9/10/11]=SPI values

################### general configuration #####################
Gain_dB=20.6            
Mode=ushort(0)          # 0=PulseEcho 1=Pitch&Catch
Compression=ushort(0)   # 0=No compression
AutoSampling=0          # 0=Manual sampling
ScaleDelay_us=0
Voltage_V=230           # Maximum=230V !
Width=11                # step ontegerf 6.25ns
PRF_kHz=0.1             # Pulse Repetition Frequency
Scale_us=50
TGC_ONOFF=ushort(1)     # 1=OFF (!)
PolarES=-60             # Echo-Start threshold (%)
Filter=ushort(4)        # 0=1.25MHz 1=2.5MHz 2=5MHz 3=10MHz 4=NoFilter=BroadBand
SamplingFreq=ushort(1)  # 0=160MHz 1=80MHz 2=40MHz 3=20MHz

NbSamples=int(Scale_us/(0.00625*pow(2,SamplingFreq)))
Array=(ctypes.c_ushort * NbSamples)(0)
tab=(ctypes.c_ushort * NbSamples)(0)

with Serial(port=Port, baudrate=115200, timeout=1, writeTimeout=1) as US_Builder:
    print ("Serial ",Port," Open")
    print()

    Gain_Bin=int(65+(Gain_dB*10))
    GainLSB=ushort(Gain_Bin%256)
    GainMSB=ushort((Gain_Bin-GainLSB)/256)
    SerialFrame[5]=Write
    SerialFrame[6]=ushort(4) # NbByte to write for SPI values
    SerialFrame[7]=ushort(0) # TGC Address       
    SerialFrame[8]=GainMSB       
    SerialFrame[9]=GainLSB       
    SerialFrame[10]=ushort(0)############################## Fct 0 = Gain       
    US_Builder.write(bytes(SerialFrame[0:11]))
    print(SerialFrame[0:11])
    time.sleep(0.01)
    
    SerialFrame[5]=Write
    SerialFrame[6]=ushort(2) # NbByte to write for SPI values
    SerialFrame[7]=Compression
    SerialFrame[8]=ushort(3) ############################## Fct 3 = Compression
    US_Builder.write(bytes(SerialFrame[0:9]))
    print(SerialFrame[0:9])
    time.sleep(0.01)
    
    AutoSampLSB=ushort(AutoSampling%256)
    AutoSampMSB=ushort((AutoSampling-AutoSampLSB)/256)
    SerialFrame[5]=Write
    SerialFrame[6]=ushort(3) # NbByte to write for SPI values
    SerialFrame[7]=AutoSampMSB
    SerialFrame[8]=AutoSampLSB
    SerialFrame[9]=ushort(4) ############################## Fct 4 = AutoSampling
    US_Builder.write(bytes(SerialFrame[0:10]))
    print(SerialFrame[0:10])
    time.sleep(0.01)
    
    ScaleDelay_Bin=int(ScaleDelay_us/0.025) # in step of 25ns
    DelayLSB=ushort(ScaleDelay_Bin%256)
    DelayMSB=ushort((ScaleDelay_Bin-DelayLSB)/256)
    SerialFrame[5]=Write
    SerialFrame[6]=ushort(3) # NbByte to write for SPI values
    SerialFrame[7]=DelayMSB
    SerialFrame[8]=DelayLSB
    SerialFrame[9]=ushort(5) ############################## Fct 5 = ScaleDelay
    US_Builder.write(bytes(SerialFrame[0:10]))
    print(SerialFrame[0:10])
    time.sleep(0.01) 

    Voltage_Bin=ushort((98*Voltage_V/180)+81.777)
    #print (Voltage_Bin)
    SerialFrame[5]=Write
    SerialFrame[6]=ushort(2) # NbByte to write for SPI values
    SerialFrame[7]=Voltage_Bin
    SerialFrame[8]=ushort(6) ############################## Fct 6 = Tx Voltage
    US_Builder.write(bytes(SerialFrame[0:9]))
    print(SerialFrame[0:9])
    time.sleep(0.01)

    SerialFrame[5]=Write
    SerialFrame[6]=ushort(2) # NbByte to write for SPI values
    SerialFrame[7]=ushort(Width)
    SerialFrame[8]=ushort(7) ############################## Fct 7 = Tx Width
    US_Builder.write(bytes(SerialFrame[0:9]))
    print(SerialFrame[0:9])
    time.sleep(0.01)

    Period_s=1/(PRF_kHz*1000)
    Period_Bin=Period_s/0.000000025
    Period_LSB=uint(Period_Bin%65536)
    Period_MSB=uint((Period_Bin-Period_LSB)/65536)
    Period_MSBL=ushort(Period_MSB%256)
    Period_LSBL=ushort(Period_LSB%256)
    Period_LSBM=ushort((Period_LSB-Period_LSBL)/256)
    #print (Period_MSBL,Period_LSBM,Period_LSBL)
    SerialFrame[5]=Write
    SerialFrame[6]=ushort(4) # NbByte to write for SPI values
    SerialFrame[7]=Period_MSBL       
    SerialFrame[8]=Period_LSBM       
    SerialFrame[9]=Period_LSBL       
    SerialFrame[10]=ushort(8) ############################## Fct 8 = PRF       
    US_Builder.write(bytes(SerialFrame[0:11]))
    print(SerialFrame[0:11])
    time.sleep(0.01)
    
    SerialFrame[5]=Write
    SerialFrame[6]=ushort(2) # NbByte to write for SPI values
    SerialFrame[7]=Mode
    SerialFrame[8]=ushort(9) ############################## Fct 9 = Mode PulseEcho or Pitch&Catch
    US_Builder.write(bytes(SerialFrame[0:9]))
    print(SerialFrame[0:9])
    time.sleep(0.01)
    
    Scale_Bin=int((Scale_us+1)/0.025) # in step of 25ns
    ScaleLSB=ushort(Scale_Bin%256)
    ScaleMSB=ushort((Scale_Bin-ScaleLSB)/256)
    SerialFrame[5]=Write
    SerialFrame[6]=ushort(3) # NbByte to write for SPI values
    SerialFrame[7]=ScaleMSB
    SerialFrame[8]=ScaleLSB
    SerialFrame[9]=ushort(10) ############################## Fct 10 = Scale
    US_Builder.write(bytes(SerialFrame[0:10]))
    print(SerialFrame[0:10])
    time.sleep(0.01) 

    SerialFrame[5]=Write
    SerialFrame[6]=ushort(2) # NbByte to write for SPI values
    SerialFrame[7]=TGC_ONOFF
    SerialFrame[8]=ushort(11) ############################## Fct 11 = TGC (DAC curve)
    US_Builder.write(bytes(SerialFrame[0:9]))
    print(SerialFrame[0:9])
    time.sleep(0.01)
    
    SerialFrame[5]=Write
    SerialFrame[6]=ushort(5) # NbByte to write for SPI values
    SerialFrame[7]=ushort(0)       
    SerialFrame[8]=ushort(0)       
    SerialFrame[9]=ushort(0)
    SerialFrame[10]=ushort(0)
    SerialFrame[11]=ushort(12) ############################## Fct 12 = Echo-Start = 0 = OFF      
    US_Builder.write(bytes(SerialFrame[0:12]))
    print(SerialFrame[0:12])
    time.sleep(0.01)
    
    Polar_Bin=ushort((PolarES*1.27)+128)
    SerialFrame[5]=Write
    SerialFrame[6]=ushort(2) # NbByte to write for SPI values
    SerialFrame[7]=Polar_Bin
    SerialFrame[8]=ushort(13) ############################## Fct 13 = Echo-Start threshold
    US_Builder.write(bytes(SerialFrame[0:9]))
    print(SerialFrame[0:9])
    time.sleep(0.01)

    SerialFrame[5]=Write
    SerialFrame[6]=ushort(2) # NbByte to write for SPI values
    SerialFrame[7]=Filter
    SerialFrame[8]=ushort(14) ############################## Fct 14 = Filter
    US_Builder.write(bytes(SerialFrame[0:9]))
    print(SerialFrame[0:9])
    time.sleep(0.01)

    # Fct 15/16/17 not used (3 gates measurement position/width)
    # Fct 18/19/20 not used (3 gates measurement threshold/alarm)
    # Fct 21/22    not used (3 gates measurement options)
    
    SerialFrame[5]=Write
    SerialFrame[6]=ushort(2) # NbByte to write for SPI values
    SerialFrame[7]=ushort(0) # 0=8bit A-scan 1=measurement 2=12bit A-scan (LSB)
    SerialFrame[8]=ushort(23) ############################## Fct 23 = ReadingPort
    US_Builder.write(bytes(SerialFrame[0:9]))
    print(SerialFrame[0:9])
    time.sleep(0.01)

    SerialFrame[5]=Write
    SerialFrame[6]=ushort(2) # NbByte to write for SPI values
    SerialFrame[7]=SamplingFreq
    SerialFrame[8]=ushort(24) ############################## Fct 24 = Sampling Frequency
    US_Builder.write(bytes(SerialFrame[0:9]))
    print(SerialFrame[0:9])
    time.sleep(0.01)

    SerialFrame[5]=Write
    SerialFrame[6]=ushort(1) # NbByte to write for SPI values
    SerialFrame[7]=ushort(2) ############################## Fct 2 = Sampling Request       
    US_Builder.write(bytes(SerialFrame[0:8]))

    time.sleep(0.5) # Wait a period of PRF

    NbSampLSB=ushort(NbSamples%256)
    NbSampMSB=ushort((NbSamples-NbSampLSB)/256)
    SerialFrame[5]=Read
    SerialFrame[6]=ushort(NbSampMSB)
    SerialFrame[7]=ushort(NbSampLSB)       
    US_Builder.write(bytes(SerialFrame[0:8]))
      
    x=np.linspace(0,NbSamples-1,NbSamples)
    y=bytearray.fromhex(US_Builder.read(NbSamples).hex())
    
    plt.ion()
    figure, ax = plt.subplots()
    line1, = ax.plot(x, y)
    plt.ylabel('8bit amplitude')
    plt.xlabel('Number of Samples')
    plt.title('A-scan')
    plt.grid(True)
    for _ in range(20):
        SerialFrame[5]=Write
        SerialFrame[6]=ushort(1) # NbByte to write for SPI values
        SerialFrame[7]=ushort(2) # Fct 2 = Sampling Request       
        #print("Fct 2 = Sampling Request *** Serial Frame=",SerialFrame[0:8])
        US_Builder.write(bytes(SerialFrame[0:8]))

        time.sleep(0.1) # Wait a period of PRF

        NbSampLSB=ushort(NbSamples%256)
        NbSampMSB=ushort((NbSamples-NbSampLSB)/256)
        SerialFrame[5]=Read
        SerialFrame[6]=ushort(NbSampMSB)
        SerialFrame[7]=ushort(NbSampLSB)       
        #print("*** Serial Frame=",SerialFrame[0:8])
        US_Builder.write(bytes(SerialFrame[0:8]))
        
        new_y=bytearray.fromhex(US_Builder.read(NbSamples).hex())
        line1.set_xdata(x)
        line1.set_ydata(new_y)
    
        figure.canvas.draw()
        figure.canvas.flush_events()
    
    print()
    US_Builder.close()
    print ("Serial ",Port," Close")

