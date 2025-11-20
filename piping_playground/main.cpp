#include <iostream>
#include "ReaderThread.h"
#include "ProcessThread.h"

int main() {
    
    // Create thread object
    ReaderThread readerThread("samples.csv");
    ProcessThread processThread;
    
    // Start the thread
    readerThread.start();
    processThread.start();
    
    // Wait for thread to complete
    readerThread.join();
    processThread.join();
        
    return 0;
}