#include "USBuilder.h"
#include <iostream>
#include <vector>


int main(){

    // Instantiate device on COM4 (adjust if needed)
    USBuilder dev("\\\\.\\COM4"); 

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
    std::vector<unsigned char> samples;
    if (dev.requestAscan8bit(512, samples)) {
        std::cout << "GOT " << samples.size() << " samples. First 10:" << std::endl;
        for (int i=0; i<10; i++) std::cout << (int)samples[i] << " ";
        std::cout << std::endl;
    } else {
        std::cerr << "ASCAN request failed" << std::endl;
    }

    // Disconnect before exiting
    dev.disconnect();
    return 0;

}

