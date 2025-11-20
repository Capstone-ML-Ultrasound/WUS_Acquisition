#include "USBuilder.h"
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip> 
#include <sstream> 
#include <format>
#include <filesystem>

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
    if (numPoints <= 0 || numPoints > 4090){
         return false;
    }

    /**
     * Command structure based on manual
     * cmd[0..4] == header 
     * cmd[5] == mode -- in this case 0, which requests A-scan read (must specify number of points cmd[6/7])
     * cmd[6] == most significant bit (high byte)
     * cmd[7] == least significant bit (low byte)
     * Big-endian -> 16 bit split 1 byte MSBs // 1 byte LSbs
     */
    unsigned char cmd[12] = {140,140,140,140,140, 0, 0,0,0,0,0,0};
    
    cmd[6] = (numPoints >> 8) & 0xFF; // high byte
    cmd[7] = numPoints & 0xFF;        // low byte

    if (!writeAll(cmd, sizeof(cmd))){
         return false;
    }

    // Read samples back
    outData.resize(numPoints);
    return readExact(outData.data(), numPoints, 5000); // .data() returns ptr to underlying vecotr 
}
/**
 * Request burst of numFrames A-scan (ultrasound intensity vs. depth).
 * @param numPoints Number of samples requested (e.g. 512)
 * @param numFrames Number of scans requested
 * @param outData Will be filled with samples from device
 * 
 * Note -- if the request is bad -- it will give all 50's back to you (dummy data)
 */
bool USBuilder::requestAscan8bitBurst(int numPoints, int numFrames, std::vector<std::vector<unsigned char>>& outBurstData) {
    if (numPoints <= 0 || numPoints > 4090){
            return false;
        }
    
    unsigned char cmd[12] = {140,140,140,140,140, 0, 0,0,0,0,0,0};

    cmd[6] = (numPoints >> 8) & 0xFF;
    cmd[7] = numPoints & 0xFF;

    outBurstData.resize(numFrames); // preallocate top most vec  
    for (auto &frame : outBurstData) frame.resize(numPoints); // preallocate inner vecs so doesnt run in loop

    for (int i = 0; i < numFrames; ++i) {
        if (!writeAll(cmd, sizeof(cmd))){
         return false; 
        }
        readExact(outBurstData[i].data(), numPoints, 5000);

    }

    return true;

    }
/**
 *  Save samples collected into csv
 */
bool USBuilder::writeCSV(std::vector<unsigned char>& samples) {
    // Obtain and format the current timestamp
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&currentTime);

    std::stringstream ss;
    ss << std::put_time(localTime, "%Y-%m-%d_%H-%M-%S"); 
    std::string timestamp = ss.str();

    // Create or ensure data directory exists 
    std::filesystem::path cwd = std::filesystem::current_path();
    std::filesystem::path data_dir = cwd / "data";

    // Create the folder if it doesn’t exist
    if (!std::filesystem::exists(data_dir)) {
        try {
            std::filesystem::create_directory(data_dir);
            std::cout << "Created directory: " << data_dir << std::endl;
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error creating directory: " << e.what() << std::endl;
            return false;
        }
    }

    // Build full file path
    std::string csv_name = "sample_" + timestamp + ".csv";
    std::filesystem::path csv_location = data_dir / csv_name;

    std::cout << "FILE NAME: " << csv_name << " Located: " << csv_location << std::endl;

    // Open file for writing 
    std::ofstream outputFile(csv_location, std::ios_base::app);
    if (!outputFile.is_open()) {
        std::cerr << "Error opening file: " << csv_location << std::endl;
        perror("Reason");
        return false;
    }

    // Write data 
    for (unsigned char s : samples) {
        outputFile << (int)s << std::endl;
    }

    outputFile.close();
    std::cout << "Data saved to " << csv_location << std::endl;
    return true;
}

bool USBuilder::writeBurstCSV(const std::vector<std::vector<unsigned char>>& burstData) {
    if (burstData.empty()) {
        std::cerr << "writeBurstCSV: burstData is empty.\n";
        return false;
    }

    // Determine max number of samples across frames
    size_t numFrames = burstData.size();
    size_t maxSamples = 0;
    for (const auto& f : burstData) maxSamples = std::max(maxSamples, f.size());
    if (maxSamples == 0) {
        std::cerr << "writeBurstCSV: all frames are empty.\n";
        return false;
    }

    // Timestamped filename under ./data/
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&currentTime);

    std::stringstream ss;
    ss << std::put_time(localTime, "%Y-%m-%d_%H-%M-%S");
    std::string timestamp = ss.str();

    std::filesystem::path data_dir = std::filesystem::current_path() / "data";
    if (!std::filesystem::exists(data_dir)) {
        try {
            std::filesystem::create_directory(data_dir);
            std::cout << "Created directory: " << data_dir << '\n';
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error creating directory: " << e.what() << '\n';
            return false;
        }
    }

    std::string csv_name = "burst_" + timestamp + ".csv";
    std::filesystem::path csv_location = data_dir / csv_name;
    std::cout << "FILE NAME: " << csv_name << " Located: " << csv_location << '\n';

    // Open NEW file (truncate) and write
    std::ofstream out(csv_location, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        std::cerr << "Error opening file: " << csv_location << '\n';
        perror("Reason");
        return false;
    }

    // Header: frame_0,frame_1,...,frame_{N-1}
    for (size_t c = 0; c < numFrames; ++c) {
        out << "frame_" << c;
        if (c + 1 < numFrames) out << ',';
    }
    out << '\n';

    // Rows: sample index 0..maxSamples-1
    for (size_t r = 0; r < maxSamples; ++r) {
        for (size_t c = 0; c < numFrames; ++c) {
            int val = (r < burstData[c].size()) ? static_cast<int>(burstData[c][r]) : 0; // pad if ragged
            out << val;
            if (c + 1 < numFrames) out << ',';
        }
        out << '\n';
    }

    out.close();
    std::cout << "Data saved to " << csv_location << '\n';
    return true;
}
