from pylab import *
from serial import *
from ctypes import *
from matplotlib import pyplot as plt
import ctypes
import time
import pyqtgraph as pg 

Port="COM4"
Header=[140,140,140,140,140]        # Lenght=5
Write=ushort(1)
Read=ushort(0)
FirmVer=ushort(3)
#                  [R/W , NbByte for SPI, 5 bytes for SPI part
SerialFrame=Header+[ 0  ,       0       , 0,0,0,0,0]  # SerialFrame[5]=R/W [6]=NbByte [7/8/9/10/11]=SPI values


with Serial(port=Port, baudrate=9600, timeout=1, writeTimeout=1) as US_Builder:
    print ("Serial ",Port," Open")
    print()

    SerialFrame[5]=FirmVer
    SerialFrame[6]=ushort(0)
    SerialFrame[7]=ushort(1)       
    print("Firm.Ver. Reading : *** Serial Frame=",SerialFrame[0:8])
    US_Builder.write(bytes(SerialFrame[0:8]))
    print ("Arduino Firm. Ver. :",int(US_Builder.read(1).hex()))
    print()   

    SerialFrame[5]=Write
    SerialFrame[6]=ushort(2) # NbByte to write for SPI values
    SerialFrame[7]=ushort(0) # 0=8bit A-scan 1=measurement 2=12bit A-scan (LSB)
    SerialFrame[8]=ushort(23) ############################## Fct 23 = ReadingPort
    US_Builder.write(bytes(SerialFrame[0:9]))

    
    SerialFrame[5]=Write
    SerialFrame[6]=ushort(1) # NbByte to write for SPI values
    SerialFrame[7]=ushort(2) # Fct 2 = Sampling Request       
    print("Fct 2 = Sampling Request *** Serial Frame=",SerialFrame[0:8])
    US_Builder.write(bytes(SerialFrame[0:8]))

    time.sleep(0.5) # Wait a period of PRF

    NbSamples=4000
    tab=(ctypes.c_uint * NbSamples)(0)
    tabMSB=(ctypes.c_ushort * NbSamples)(0)
    tabLSB=(ctypes.c_ushort * NbSamples)(0)
    
    NbSampLSB=ushort(NbSamples%256)
    NbSampMSB=ushort((NbSamples-NbSampLSB)/256)
    SerialFrame[5]=Read
    SerialFrame[6]=ushort(NbSampMSB)
    SerialFrame[7]=ushort(NbSampLSB)       
    print("*** Serial Frame=",SerialFrame[0:8])
    
    US_Builder.write(bytes(SerialFrame[0:8]))
    
    tab[0]=0
    tab[NbSamples-1]=4095
    x=np.linspace(0,NbSamples-1,NbSamples)
    y=tab
    plt.ion()
    figure, ax = plt.subplots()
    line1, = ax.plot(x, y)
    plt.title("A-scan", fontsize=20)
    plt.ylabel('8bit amplitude')
    plt.xlabel("NbSamples")
    plt.grid(True)

    for _ in range(10):
        SerialFrame[5]=Write
        SerialFrame[6]=ushort(1) # NbByte to write for SPI values
        SerialFrame[7]=ushort(2) # Fct 2 = Sampling Request       
        US_Builder.write(bytes(SerialFrame[0:8]))

        time.sleep(0.01) # Wait a period of PRF

        NbSampLSB=ushort(NbSamples%256)
        NbSampMSB=ushort((NbSamples-NbSampLSB)/256)
        SerialFrame[5]=Read 
        SerialFrame[6]=ushort(NbSampMSB)
        SerialFrame[7]=ushort(NbSampLSB)       
        US_Builder.write(bytes(SerialFrame[0:8]))
        tabMSB=bytearray.fromhex(US_Builder.read(NbSamples).hex())

        SerialFrame[5]=Write
        SerialFrame[6]=ushort(2) # NbByte to write for SPI values
        SerialFrame[7]=ushort(2) # 0=8bit A-scan 1=measurement 2=12bit A-scan (LSB)
        SerialFrame[8]=ushort(23) ############################## Fct 23 = ReadingPort
        US_Builder.write(bytes(SerialFrame[0:9]))

        NbSampLSB=ushort(NbSamples%256)
        NbSampMSB=ushort((NbSamples-NbSampLSB)/256)
        SerialFrame[5]=Read 
        SerialFrame[6]=ushort(NbSampMSB)
        SerialFrame[7]=ushort(NbSampLSB)       
        US_Builder.write(bytes(SerialFrame[0:8]))
        tabLSB=bytearray.fromhex(US_Builder.read(NbSamples).hex())
        
        SerialFrame[5]=Write
        SerialFrame[6]=ushort(2) # NbByte to write for SPI values
        SerialFrame[7]=ushort(0) # 0=8bit A-scan 1=measurement 2=12bit A-scan (LSB)
        SerialFrame[8]=ushort(23) ############################## Fct 23 = ReadingPort
        US_Builder.write(bytes(SerialFrame[0:9]))

        #new_y=[i*16 for i in tabMSB]#+tabLSB
        for i in range(0,NbSamples):
            tab[i]=(tabMSB[i]*16)+(tabLSB[i] & 15)
        new_y=tab
        
        line1.set_xdata(x)
        line1.set_ydata(new_y)
    
        figure.canvas.draw()
        figure.canvas.flush_events()
    

    
    print()
    US_Builder.close()
    print ("Serial ",Port," Close")

