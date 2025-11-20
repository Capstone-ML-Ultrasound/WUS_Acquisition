#ifndef PROCESS_THREAD_H
#define PROCESS_THREAD_H

#include <thread>

class ProcessThread {
public:
    // Constructor
    ProcessThread();
    
    // Destructor
    ~ProcessThread();
    
    // Method that will run in the thread
    void run();
    
    // Start the thread
    void start();
    
    // Wait for thread to finish
    void join();

    void process();
    
private:
    std::thread t;
};

#endif