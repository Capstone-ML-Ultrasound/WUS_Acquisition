#include "ReaderThread.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>

ReaderThread::ReaderThread(const std::string& filename) 
    : csvFilename(filename) {}

ReaderThread::~ReaderThread() {
    if (t.joinable()) {
        t.join();
    }
}

std::vector<int> ReaderThread::readCSV() {
    std::vector<int> values;
    std::ifstream file(csvFilename);
    
    if (!file.is_open()) {
        std::cerr << "[READER] - Error: Could not open file " << csvFilename << std::endl;
        return values;
    }
    
    int value;
    while (file >> value) {
        values.push_back(value);
    }
    
    file.close();
    return values;
}

void ReaderThread::stream() {
    std::cout << "[READER] - Thread ID: " << std::this_thread::get_id() << std::endl;
    

    // config steps for hard ware 

    /**
     * main loop 
     * 
     * 1. get data 
     * 2. send over to buffer/kafka topic 
     * 
     */
    for (int i = 0; i < 10; i++) {

        // 1. get data (simulated)
        std::vector<int> values = readCSV();
        std::cout << "[READER] - Iteration " << (i + 1) << ": [";
        for (size_t j = 0; j < values.size(); j++) {
            std::cout << values[j];
            if (j < values.size() - 1) {
                std::cout << " ";
            }
        }
        std::cout << "]" << std::endl;
        
        //small deplay 
        std::this_thread::sleep_for(std::chrono::milliseconds(500));



        // 2. Send data (use values var) over to buffer/ kafka 
        // also see if you can store the raw data somewhere for now so we use it when doing 
        // preprocessing steps. Just to someone can do it without needed hardware 

    }
}

void ReaderThread::run() {
    stream();
}

void ReaderThread::start() {
    t = std::thread(&ReaderThread::run, this);
}

void ReaderThread::join() {
    if (t.joinable()) {
        t.join();
    }
}
