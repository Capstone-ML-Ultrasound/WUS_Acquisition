#ifndef USBUILDER_H
#define USBUILDER_H
#include <string>
#include <vector>
#include <windows.h>


class USBuilder{
public: 
    USBuilder(const std::string& portName);
    ~USBuilder();

    bool connect();
    void disconnect();

    // bool requestFirmware(std::string& versionOut);
    // bool requestAscan8bit(int numPoints, std::vector<unsigned char>& outData);

private:
    std::string m_portName;
    HANDLE m_handle;

    // bool writeAll(const unsigned char* buf, size_t len);
    // bool readExact(unsigned char*buf, size_t len, DWORD timeoutMS=2000);

};
#endif 