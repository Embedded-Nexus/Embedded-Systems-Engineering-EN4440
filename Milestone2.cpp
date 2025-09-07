#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include <curl/curl.h>


using namespace std;

struct RegisterInfo {
    string name;
    uint16_t gain;
    string unit;
    bool writable;
};

vector<RegisterInfo> registerMap = {
    {"Vac1 / L1 Phase voltage", 10, "V", false},
    {"Iac1 / L1 Phase current", 10, "A", false},
    {"Fac1 / L1 Phase frequency", 100, "Hz", false},
    {"Vpv1 / PV1 input voltage", 10, "V", false},
    {"Vpv2 / PV2 input voltage", 10, "V", false},
    {"Ipv1 / PV1 input current", 10, "A", false},
    {"Ipv2 / PV2 input current", 10, "A", false},
    {"Inverter internal temperature", 10, "°C", false},
    {"Export power percentage", 1, "%", true},
    {"Pac L / Inverter output power", 1, "W", false}
};


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

bool validateCRC(const vector<uint8_t>& responseFrame){
    uint16_t receivedCRC = responseFrame[responseFrame.size()-2] | (responseFrame[responseFrame.size()-1] << 8);
    uint16_t calculatedCRC = modbusCRC((uint8_t*)responseFrame.data(), responseFrame.size() - 2);
    return receivedCRC == calculatedCRC;
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

void decodeResponseFrame(const vector<uint8_t>& validresponseFrame, uint16_t startAddr){
    uint8_t funcCode = validresponseFrame[1];

    if(funcCode == 0x03){ // Read Holding Registers
        uint8_t byteCount = validresponseFrame[2];
        uint8_t numRegisters = byteCount / 2;

        for(uint16_t j = 0; j < numRegisters; j++){
            uint16_t regAddr = startAddr + j; // actual register address
            uint16_t rawValue = (validresponseFrame[3 + j*2] << 8) |
                                 validresponseFrame[4 + j*2];

            if (regAddr < registerMap.size()) {
                const auto& regInfo = registerMap[regAddr];
                double scaledValue = static_cast<double>(rawValue) / regInfo.gain;

                cout << "Register " << regAddr 
                     << " (" << regInfo.name << "): "
                     << scaledValue << " " << regInfo.unit
                     << " [raw=" << rawValue << "]" << endl;
            } else {
                cout << "Register " << regAddr << ": " << rawValue << " (Unknown)" << endl;
            }
        }
    }
    else if(funcCode == 0x06){ // Write Single Register
        uint16_t addr = (validresponseFrame[2] << 8) | validresponseFrame[3];
        uint16_t value = (validresponseFrame[4] << 8) | validresponseFrame[5];

        if (addr < registerMap.size()) {
            const auto& regInfo = registerMap[addr];
            cout << "Write success -> " << regInfo.name
                 << " (Reg " << addr << ") set to "
                 << value << " " << regInfo.unit << endl;
        } else {
            cout << "Write success -> Address " << addr 
                 << " set to " << value << endl;
        }
    }
    else{
        cout << "Function Code not supported for decoding." << endl;
    }
}



string ValidateResponseFrame(vector<uint8_t> responseFrame){
    //Request frame is invalid
    if(responseFrame.empty()){
        cerr << "Error: Invalid Request Frame." << endl;
    }
    else{
        if(validateCRC(responseFrame)){
                cout << "CRC Check Passed." << endl;
            //Request frame is valid but requested data is invalid
            uint8_t funcCode  = responseFrame[1];
            if (funcCode & 0x80) {
                uint8_t exceptionCode = responseFrame[2];
                switch (exceptionCode) {
                    case 0x01:
                        cerr << "Error: Illegal Function." << endl;
                        break;
                    case 0x02:
                        cerr << "Error: Illegal Data Address." << endl;
                        break;
                    case 0x03:
                        cerr << "Error: Illegal Data Value." << endl;
                        break;
                    case 0x04:
                        cerr << "Error: Slave Device Failure." << endl;
                        break;
                    default:
                        cerr << "Error: Unknown Exception Code." << endl;
                        break;
                }
            }
            else{
                //Valid response frame
                cout << "Valid Response Frame Received." << endl;
                return "validResponseFrame";
            }
        }
        else{
            cerr << "Error: CRC Check Failed." << endl;
        }
    }
    return "invalidResponseFrame";
}

// void writetoInverter(){
//     writeAPI();
// }
// void readfromInverter(){
//     readAPI();
// }

////////////Functions Required for CloudAPI Inverter Sim//////////////

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

vector<uint8_t> jsontoFrame(string response){
    //extract the hex string from the JSON response
    size_t start = response.find("\"frame\":\"");
    if(start == string::npos) {
        cerr << "Error: 'frame' key not found in JSON response." << endl;
        return {};
    }

    start += 9;
    size_t end = response.find("\"", start);
    if(end == string::npos) {
        cerr << "Error: Invalid JSON format." << endl;
        return {};
    }

    string hexString = response.substr(start, end - start);
    cout << "Extracted Hex String: " << hexString << endl;
    vector<uint8_t> frame;
    // Extract hex string from JSON response
    for(int i=0; i<hexString.size(); i+=2){
        uint8_t byte = stoi(hexString.substr(i, 2), nullptr, 16);
        frame.push_back(byte);
    }
    // Parse JSON to extract hex string
    // Convert hex string back to byte vector
    return frame;
}

//Helper function for libCurl to handle response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

string readAPI(const string& jsonFrame, const string& apiKey) {
    CURL* curl;
    CURLcode res;
    string response;

    curl = curl_easy_init();
    if(curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        string authHeader = "Authorization: " + apiKey;
        headers = curl_slist_append(headers, authHeader.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, "http://20.15.114.131:8080/api/inverter/read");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonFrame.c_str());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " 
                      << curl_easy_strerror(res) << endl;
        } else {
            // cout << "Response: " << response << endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    return response;
}

string writeAPI(const string& jsonFrame, const string& apiKey) {
    CURL* curl;
    CURLcode res;
    string response;

    curl = curl_easy_init();
    if(curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        string authHeader = "Authorization: " + apiKey;
        headers = curl_slist_append(headers, authHeader.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, "http://20.15.114.131:8080/api/inverter/write");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonFrame.c_str());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " 
                      << curl_easy_strerror(res) << endl;
        } else {
            // cout << "Response: " << response << endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    return response;
}

//Main Function of the Program
int main() {
    // API Key
    string apiKey = "NjhhZWIwNDU1ZDdmMzg3MzNiMTQ5Yjg2OjY4YWViMDQ1NWQ3ZjM4NzMzYjE0OWI3Yw==";
    string response;
    uint16_t startAddr = 5; // Starting address for reading registers
    // Example usage
    vector<uint8_t> requestFrame = BuildRequestFrame(0x11, 0x03, 0x0005, 0x0005);
    string jsonFrame = frameToJson(requestFrame);
    response = readAPI(jsonFrame, apiKey);
    //response = writeAPI(jsonFrame, apiKey);
    vector<uint8_t> responseFrame = jsontoFrame(response);
    if(ValidateResponseFrame(responseFrame) == "validResponseFrame"){
        decodeResponseFrame(responseFrame, startAddr);
    }
    
    cout << response << endl;
    return 0;
}