
// We use the digital port D0~7 to drive different signals

#define E_MOSI 2      // pin D2 for Enable_MOSI
#define E_MISO 3      // pin D3 for Enable_MISO

#define mabt 5        // pin D5 for Enable Bluetooth
#define analogPin A0  // Battery level

#define ADDR_Gain                     0
#define ADDR_Mode                     1
#define ADDR_Start                    2
#define ADDR_Compres                  3 
#define ADDR_NbSamples                4
#define ADDR_Delay                    5
#define ADDR_Voltage                  6
#define ADDR_Width                    7
#define ADDR_Prf                      8
#define ADDR_ModePE                   9   
#define ADDR_Scale                    10
#define ADDR_ONOFFDAC                 11 
#define ADDR_PosES                    12  
#define ADDR_PolarES                  13
#define ADDR_Filter                   14
#define ADDR_Gate1PosWidth            15
#define ADDR_Gate2PosWidth            16 
#define ADDR_Gate3PosWidth            17 
#define ADDR_Gate1ThresholdAlFilter   18
#define ADDR_Gate2ThresholdAlFilter   19
#define ADDR_Gate3ThresholdAlFilter   20
#define ADDR_AlDelay                  21
#define ADDR_AnalogOutput             22 
#define ADDR_ReadPort                 23
#define ADDR_SamplingFreq             24
#define ADDR_ChannelTxRx              25

#define BROADBAND   4
#define F_10MHz     3    
#define F_5MHz      2
#define F_2MHz      1 // 2.5MHz  !
#define F_1MHz      0 // 1.25MHz !

#define F_160MHz    0
#define F_80MHz     1
#define F_40MHz     2
#define F_20MHz     3

#define ANALOG_OFF    0
#define ANALOG_TOTAL  1
#define ANALOG_OVER   2

#define BOTH_WAVES    0
#define NEGATIVE_WAVE 1
#define POSITIVE_WAVE 2

#define ASCAN_8bit  0
#define MEASURES    1
#define ASCAN_4LSB  2
#define EDGE_TOF    3
