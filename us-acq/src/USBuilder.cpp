#include "USBuilder.h"
#include <iostream>
#include <string>

USBuilder::USBuilder(const std::string& portName): m_portName(portName), m_handle(INVALID_HANDLE_VALUE){}
USBuilder::~USBuilder() { disconnect(); }

/**
 * Opens the COM port and configures it for communication with US-Builder.
 */
bool USBuilder::connect(){
    std::cout << "Connecting to US-Builder" << std::endl;

    // Open the COM port for read/write access
    m_handle = CreateFileA(
        m_portName.c_str(),               // COM port string
        GENERIC_READ | GENERIC_WRITE,     // read & write access
        0, NULL, OPEN_EXISTING, 0, NULL   // no sharing, must already exist
    );

    if (m_handle == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open port " << m_portName << std::endl;
        return false;
    }


    // Configure serial parameters
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(m_handle, &dcbSerialParams)){
        return false;
    }

    // Settings must match the device firmware
    dcbSerialParams.BaudRate = CBR_115200; 
    dcbSerialParams.ByteSize = 8;          
    dcbSerialParams.StopBits = ONESTOPBIT; 
    dcbSerialParams.Parity = NOPARITY;   

    if (!SetCommState(m_handle, &dcbSerialParams)){
         return false;
    }

    // Set timeouts (important for preventing infinite blocking reads)
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;         // max gap between bytes
    timeouts.ReadTotalTimeoutConstant = 2000;  // overall read timeout
    timeouts.WriteTotalTimeoutConstant = 2000; // write timeout
    SetCommTimeouts(m_handle, &timeouts);

    std::cout << "Connecting to US-Builder successfully" << std::endl;
    return true;

}


/** 
 * Disconnect from serial port for cleanup
*/
void USBuilder::disconnect() {
    if (m_handle != INVALID_HANDLE_VALUE) {
        std::cout << "Disconnected from US-Builder successfully" << std::endl;
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}  


/**
 * Write an entire buffer to the device.
 */
bool USBuilder::writeAll(const unsigned char* buf, size_t len) {
    DWORD written;
    return WriteFile(m_handle, buf, (DWORD)len, &written, NULL) && written == len;
}

/**
 * Read exactly 'len' bytes from the device.
 * Loops until all data arrives or timeout hits.
 */
bool USBuilder::readExact(unsigned char* buf, size_t len, DWORD timeoutMs) {
    DWORD total = 0;
    DWORD readBytes;
    while (total < len) {
        if (!ReadFile(m_handle, buf + total, len - total, &readBytes, NULL)){
             return false;
        }
        if (readBytes == 0) {
            break; // no data → timeout
        }
        total += readBytes;
    }
    return total == len;
}


/**
 * Send the firmware request command and read back response.
 */
bool USBuilder::requestFirmware(std::string& versionOut) {
     /** 
     * Command structure based on manual
     * cmd[0..4] == header 
     * cmd[5] == mode - in this case 3, which requests firmware version (handshake)
     * cmd[6..11] == 0 - not needed for firmware req
     */
    unsigned char cmd[12] = {140,140,140,140,140, 3, 0,0,0,0,0,0};

    //Clear previous buffer 
    PurgeComm(m_handle, PURGE_RXCLEAR | PURGE_TXCLEAR);

    if (!writeAll(cmd, sizeof(cmd))) {
        std::cout << "Failed in writing cmd to buffer" << std::endl;
        return false;
    }

    // give device time to respond
    Sleep(200); 

    unsigned char buf[1];
    if (!readExact(buf, 1, 1000)) {
        std::cout << "Failed in reading from buffer" << std::endl;
        return false;
    }

    versionOut = std::to_string((int)buf[0]); // convert numeric FW version to string

    return true;
}

/**
 * Request an A-scan (ultrasound intensity vs. depth).
 * @param numPoints Number of samples requested (e.g. 512)
 * @param outData Will be filled with samples from device
 * 
 * NOte -- if the request is bad -- it will give all 50's back to you (dummy data)
 */
bool USBuilder::requestAscan8bit(int numPoints, std::vector<unsigned char>& outData) {
    if (numPoints <= 0 || numPoints > 4000){
         return false;
    }

    /**
     * Command structure based on manual
     * cmd[0..4] == header 
     * cmd[5] == mode -- in this case 0, which requests A-scan read (must specify number of points cmd[6/7])
     * cmd[6] == most significant bit (high byte)
     * cmd[7] == least significant bit (low byte)
     */
    unsigned char cmd[12] = {140,140,140,140,140, 0, 0,0,0,0,0,0};
    cmd[6] = (numPoints >> 8) & 0xFF; // high byte
    cmd[7] = numPoints & 0xFF;        // low byte

    if (!writeAll(cmd, sizeof(cmd))){
         return false;
    }

    // Read samples back
    outData.resize(numPoints);
    return readExact(outData.data(), numPoints, 5000);
}






