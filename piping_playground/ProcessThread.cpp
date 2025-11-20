#include "ProcessThread.h"
#include <iostream>

ProcessThread::ProcessThread() {}

ProcessThread::~ProcessThread() {
    // Make sure thread is joined before destruction
    if (t.joinable()) {
        t.join();
    }
}

void ProcessThread::run() {
    process();
}

void ProcessThread::start() {
    // Create thread and pass the run method
    t = std::thread(&ProcessThread::run, this);
}

void ProcessThread::join() {
    if (t.joinable()) {
        t.join();
    }
}

void ProcessThread::process(){
    std::cout << "[PROCESS] - Thread ID: " << std::this_thread::get_id() << std::endl;

    /**
     * 1. read from buffer/kafka shared with ReaderThread
     * 2. simulate Processing (for now)
     * 3. send to kafka/buffer outwords to ML Model/ data pool
     */

}