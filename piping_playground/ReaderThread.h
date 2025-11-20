#ifndef READER_THREAD_H
#define READER_THREAD_H

#include <thread>
#include <string>
#include <vector>

class ReaderThread {
public:
    // Constructor
    ReaderThread(const std::string& filename);
    
    // Destructor
    ~ReaderThread();
    
    // Method that will run in the thread
    void run();
    
    // Start the thread
    void start();
    
    // Wait for thread to finish
    void join();


    
private:
    std::thread t;
    std::string csvFilename;
    
    std::vector<int> readCSV();
    void stream();
};

#endif