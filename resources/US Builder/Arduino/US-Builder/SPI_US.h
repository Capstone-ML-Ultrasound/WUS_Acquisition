
void SEND_BYTES(byte Len);
void ReadSamples(int passe,int Nb, byte type);

void SEND_GAIN_dB(double Gain_dB);
void SEND_GAIN_bin(unsigned int Gain_bin);

void SEND_COMPRESS_bin(byte Compress_bin);

void SEND_DELAY_us(double Delay_us);
void SEND_DELAY_bin(unsigned int Delay_bin);

void SEND_VOLTAGE_Volt(double Voltage_V);
void SEND_VOLTAGE_bin(byte Voltage_bin);

void SEND_WIDTH_ns(double Width_ns);
void SEND_WIDTH_bin(byte Width_bin);

void SEND_PRF_kHz(double PRF_kHz);
void SEND_PRF_bin(long PRF_bin);

void SEND_SCALE_us(double Scale_us);
void SEND_SCALE_bin(unsigned int Scale_bin);

void SEND_FILTER_bin(byte Filter_bin);

void SEND_SAMPLINGFREQ_bin(byte SamplingFreq_bin);

void SEND_CHANNEL_bin(byte Tx, byte Rx);

void SEND_ECHOSTARTPOSWIDTH_bin(unsigned int Pos_bin, unsigned int Width_bin);
void SEND_ECHOSTARTPOSWIDTH_us(double Pos_us, double Width_us);

void SEND_ECHOSTARTPOLARITY_bin(byte Polarity_bin);
void SEND_ECHOSTARTPOLARITY_pourcent(double Polarity_pourcent);

void SEND_GATE1POSWIDTH_us(double Pos_us, double Width_us);
void SEND_GATE1POSWIDTH_bin(unsigned int Pos_bin, unsigned int Width_bin);
void SEND_GATE2POSWIDTH_us(double Pos_us, double Width_us);
void SEND_GATE2POSWIDTH_bin(unsigned int Pos_bin, unsigned int Width_bin);
void SEND_GATE3POSWIDTH_us(double Pos_us, double Width_us);
void SEND_GATE3POSWIDTH_bin(unsigned int Pos_bin, unsigned int Width_bin);

void SEND_GATE1THRESHOLDALFILTER_pourcent(byte AlFilter, double Threshold_pourcent);
void SEND_GATE1THRESHOLDALFILTER_bin(byte AlFilter_bin, byte Threshold_bin);
void SEND_GATE2THRESHOLDALFILTER_pourcent(byte AlFilter, double Threshold_pourcent);
void SEND_GATE2THRESHOLDALFILTER_bin(byte AlFilter_bin, byte Threshold_bin);
void SEND_GATE3THRESHOLDALFILTER_pourcent(byte AlFilter, double Threshold_pourcent);
void SEND_GATE3THRESHOLDALFILTER_bin(byte AlFilter_bin, byte Threshold_bin);

void SEND_ANALOGSETPOLARITY_bin(byte Analog1Set, byte Analog2Set, byte Analog3Set, byte Polar_bin);

void SEND_NBSAMPLES_bin(unsigned int NbSamples);

void SEND_READPORT(byte ReadPort);

void SEND_ONOFFDAC(byte ONOFFDAC);

void SEND_MODE(byte MODE);

void SEND_ALDELAY_bin(unsigned int Duration, byte AlarmSet);
