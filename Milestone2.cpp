#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstdint>

using namespace std;

void SendReadFrame(){}

void extractFrame(){}
void CRCCheck(){}
void decoder(){}

void ReadDataFrame(){
    extractFrame();
    CRCCheck();
    decoder();
}


void Scheduler(){}

void BuildWriteFrame(){}
void WriteandVerify(){}
void errorPolicy(){}

void demo(){}

// CRC16 (Modbus)
uint16_t modbusCRC(uint8_t *buf, int len) {
    uint16_t crc = 0xFFFF;
    for (int pos = 0; pos < len; pos++) {
        crc ^= buf[pos];
        for (int i = 0; i < 8; i++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

//Build the read frame in JSON format
string BuildReadFrame(uint8_t slaveAddr, uint8_t funcCode, uint16_t startAddr, uint16_t numReg){
    
    // Construct the frame(slave address, function code, start address, number of registers)
    uint8_t frame[6]={
        slaveAddr,
        funcCode,
        static_cast<uint8_t>((startAddr >> 8) & 0xFF),
        static_cast<uint8_t>(startAddr & 0xFF),
        static_cast<uint8_t>((numReg >> 8) & 0xFF),
        static_cast<uint8_t>(numReg & 0xFF)
    };

    // Calculate CRC
    uint16_t crc = modbusCRC(frame, 6);

    // Convert to hex string
    ostringstream oss;
    oss << uppercase << hex << setfill('0');

    for(int i=0; i<6; i++){
        oss << setw(2) << (int)frame[i];
    }
    oss << setw(2) << (crc & 0xFF);        // low byte
    oss << setw(2) << ((crc >> 8) & 0xFF); // high byte

    string hexFrame = oss.str();

    // Wrap into JSON
    string jsonFrame = "{\"frame\":\"" + hexFrame + "\"}";
    cout << jsonFrame << endl;
    return jsonFrame;
}

int main() {
    // Example usage
    BuildReadFrame(0x01, 0x03, 0x0000, 0x0002);
    return 0;
}