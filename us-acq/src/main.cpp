#include "USBuilder.h"
#include <iostream>
#include <chrono>
#include <vector>


int main(){

    // Instantiate device on COM4 (adjust if needed)
    USBuilder dev("\\\\.\\COM3"); // TODO automate to automatically find correct port 

    // Connect to device
    if (!dev.connect()){ 
        return 1;
    }

    // Step 1: Get firmware version
    std::string version;
    if (dev.requestFirmware(version))
        std::cout << "GOT Firmware: " << version << std::endl;
    else
        std::cerr << "Firmware request failed" << std::endl;

    // Step 2: Acquire a simple A-scan of 512 points
    std::chrono::time_point<std::chrono::high_resolution_clock> end;
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<unsigned char> samples;
    
    if (dev.requestAscan8bit(512, samples)) {
        end = std::chrono::high_resolution_clock::now();
        dev.writeCSV(samples);

    } else {
        std::cerr << "ASCAN request failed" << std::endl;
    }

    std::vector<std::vector<unsigned char>> burstData;

    if (dev.requestAscan8bitBurst(512, 100, burstData)) {
        dev.writeBurstCSV(burstData);

    } else {
        std::cerr << "ASCAN Burst request failed" << std::endl;
    }

    // Disconnect before exiting
    dev.disconnect();
    
    std::chrono::duration<double, std::milli> duration_ms = end - start;
    std::cout << "Time for a single scan 512 pts scan: " << duration_ms.count() << "ms" << std::endl;
    
    return 0;

}

