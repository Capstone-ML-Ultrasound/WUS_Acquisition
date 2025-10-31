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

    bool requestFirmware(std::string& versionOut);
    bool requestAscan8bit(int numPoints, std::vector<unsigned char>& outData);
    bool requestAscan8bitBurst(int numPoints, int numFrames, std::vector<std::vector<unsigned char>>& outData);
    bool writeCSV(std::vector<unsigned char>& samples);
    bool writeBurstCSV(const std::vector<std::vector<unsigned char>>& burstData); // TODO MERGE 2 CSV WRITING FUNTIONS TOGETHER 

private:
    std::string m_portName;
    HANDLE m_handle;

    bool writeAll(const unsigned char* buf, size_t len);
    bool readExact(unsigned char*buf, size_t len, DWORD timeoutMS=2000);

};
#endif 