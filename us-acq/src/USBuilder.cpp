#include "USBuilder.h"
#include <iostream>
#include <string>

USBuilder::USBuilder(const std::string& portName): m_portName(portName), m_handle(INVALID_HANDLE_VALUE){}
USBuilder::~USBuilder() { disconnect(); }

bool USBuilder::connect(){
    std::cout << "Hello World!" << std::endl;

    return false;
}


void USBuilder::disconnect(){
    std::cout << "Bye World!" << std::endl;

}





