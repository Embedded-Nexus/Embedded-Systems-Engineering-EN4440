#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
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

//Build the Request Frame
vector<uint8_t> BuildRequestFrame(uint8_t slaveAddr, uint8_t funcCode, uint16_t startAddr, uint16_t numReg){
    // Construct the frame(slave address, function code, start address, number of registers)
    vector<uint8_t> frame(8);

        frame[0] = slaveAddr;
        frame[1] = funcCode;
        frame[2] = static_cast<uint8_t>((startAddr >> 8) & 0xFF);
        frame[3] = static_cast<uint8_t>(startAddr & 0xFF);
        frame[4] = static_cast<uint8_t>((numReg >> 8) & 0xFF);
        frame[5] = static_cast<uint8_t>(numReg & 0xFF);

        // Calculate CRC
        uint16_t crc = modbusCRC(frame.data(), 6);
        // Append CRC to frame
        frame[6] = crc & 0xFF;        // low byte
        frame[7] = (crc >> 8) & 0xFF; // high byte
        return frame;
}


string frameToJson(vector<uint8_t> frame){
    // Convert to hex string
    ostringstream oss;
    // Format settings(Upper case and zero padding)
    oss << uppercase << hex << setfill('0');
    //Loop over the frame to convert each byte to hex
    for(int i=0; i<frame.size(); i++){ // Update loop to use frame.size()
        oss << setw(2) << (int)frame[i];
    }
    string hexFrame = oss.str();
    // Wrap into JSON
    string jsonFrame = "{\"frame\":\"" + hexFrame + "\"}";
    return jsonFrame;
}

int main() {
    // Example usage
    vector<uint8_t> requestFrame = BuildRequestFrame(0x01, 0x03, 0x0000, 0x0002);
    string jsonFrame = frameToJson(requestFrame);
    cout << jsonFrame << endl;

    return 0;
}