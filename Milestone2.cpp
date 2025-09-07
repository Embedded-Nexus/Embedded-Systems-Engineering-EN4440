#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include <curl/curl.h>
#include <chrono>
#include <thread>
#include <ctime>

using namespace std;

struct RegisterInfo {
    string name;
    uint16_t gain;
    string unit;
    bool writable;
};

struct Sample {
    chrono::system_clock::time_point timestamp;
    uint16_t regAddr;
    double value;
};

vector<Sample> dataBuffer;

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

// error logger
void logError(const string& msg) {
    auto now = chrono::system_clock::now();
    time_t now_c = chrono::system_clock::to_time_t(now);
    tm local_tm{};
    localtime_s(&local_tm, &now_c); 
    cerr << "[ERROR] " << put_time(&local_tm, "%H:%M:%S")
         << " - " << msg << endl;
}

// info logger
void logInfo(const string& msg) {
    auto now = chrono::system_clock::now();
    time_t now_c = chrono::system_clock::to_time_t(now);
    tm local_tm{};
    localtime_s(&local_tm, &now_c);
    cout << put_time(&local_tm, "%H:%M:%S")
         << " - " << msg << endl;
}

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

//CRC Validation
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

//Decode the Response Frame
void decodeResponseFrame(const vector<uint8_t>& validresponseFrame, uint16_t startAddr){
    uint8_t funcCode = validresponseFrame[1];

    if(funcCode == 0x03){ // Read Holding Registers
        uint8_t byteCount = validresponseFrame[2];
        uint8_t numRegisters = byteCount / 2;

        auto snapshotTime = chrono::system_clock::now();  
        vector<Sample> snapshot;  // collect this read’s samples

        for(uint16_t j = 0; j < numRegisters; j++){
            uint16_t regAddr = startAddr + j;
            uint16_t rawValue = (validresponseFrame[3 + j*2] << 8) |
                                 validresponseFrame[4 + j*2];

            if (regAddr < registerMap.size()) {
                const auto& regInfo = registerMap[regAddr];
                double scaledValue = static_cast<double>(rawValue) / regInfo.gain;

                // cout << "Register " << regAddr 
                //      << " (" << regInfo.name << "): "
                //      << scaledValue << " " << regInfo.unit
                //      << " [raw=" << rawValue << "]" << endl;

                // add to snapshot instead of pushing immediately
                snapshot.push_back({ snapshotTime, regAddr, scaledValue });
            } else {
                cout << "Register " << regAddr << ": " << rawValue << " (Unknown)" << endl;
            }
        }

        // after collecting, append entire snapshot into dataBuffer
        dataBuffer.insert(dataBuffer.end(), snapshot.begin(), snapshot.end());
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


//Response Frame Validation
string ValidateResponseFrame(vector<uint8_t> responseFrame){
    //Request frame is invalid
    if(responseFrame.empty()){
        cerr << "Error: Invalid Request Frame." << endl;
    }
    else{
        if(validateCRC(responseFrame)){
            logInfo("CRC Check Passed");
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
                // cout << "Valid Response Frame Received." << endl;
                return "validResponseFrame";
            }
        }
        else{
            cerr << "Error: CRC Check Failed." << endl;
        }
    }
    return "invalidResponseFrame";
}

string readfromInverter(const string& jsonFrame, const string& apiKey, int maxRetries = 3) {
    for (int attempt = 1; attempt <= maxRetries; attempt++) {
        string resp = readAPI(jsonFrame, apiKey);

        if (!resp.empty()) {
            auto respFrame = jsontoFrame(resp);
            if (ValidateResponseFrame(respFrame) == "validResponseFrame") {
                logInfo("Valid Response Frame Received");
                logInfo("Read successful on attempt " + to_string(attempt));
                return resp; // success
            }
        }

        logError("Read attempt " + to_string(attempt) + " failed. Retrying...");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    logError("All retries failed. Giving up.");
    return "";
}

string writetoInverter(const string& jsonFrame, const string& apiKey, int maxRetries = 3) {
    for (int attempt = 1; attempt <= maxRetries; attempt++) {
        string resp = writeAPI(jsonFrame, apiKey);

        if (!resp.empty()) {
            auto respFrame = jsontoFrame(resp);
            if (ValidateResponseFrame(respFrame) == "validResponseFrame") {
                logInfo("Write successful on attempt " + to_string(attempt));
                return resp; 
            }
        }

        logError("Write attempt " + to_string(attempt) + " failed. Retrying...");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    logError("All write retries failed. Giving up.");
    return "";
}

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
    }

    start += 9;
    size_t end = response.find("\"", start);
    if(end == string::npos) {
        cerr << "Error: Invalid JSON format." << endl;
    }

    string hexString = response.substr(start, end - start);
    //cout << "Extracted Hex String: " << hexString << endl;
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

struct TestCase {
    vector<uint8_t> frame;
    uint16_t startAddr;
};

// Function to build and return test cases
TestCase getTestCase(size_t index) {
    switch (index) {
        case 0:  return { BuildRequestFrame(0x11, 0x03, 0x0005, 0x0005), 5 };      // Normal read
        case 1:  return { BuildRequestFrame(0x11, 0x03, 0x0000, 0x0002), 0 };      // Edge read
        case 2:  return { BuildRequestFrame(0x11, 0x03, 0x0008, 0x000A), 8 };      // Larger read
        case 3:  return { BuildRequestFrame(0x11, 0x03, 0xFFFF, 0x0001), 0 };      // Invalid read
        case 4:  return { BuildRequestFrame(0x11, 0x03, 0x0005, 0x0001), 5 };      // Smallest read
        case 5:  return { BuildRequestFrame(0x11, 0x03, 0x0002, 0x0003), 2 };      // Mid-range read
        case 6:  return { BuildRequestFrame(0x11, 0x03, 0x0007, 0x0002), 7 };      // Read near writable zone

        case 7:  return { BuildRequestFrame(0x11, 0x06, 0x0008, 0x0032), 8 };      // Valid write (50%)
        case 8:  return { BuildRequestFrame(0x11, 0x03, 0x0008, 0x0001), 5 };      // Normal read
        case 9:  return { BuildRequestFrame(0x11, 0x06, 0x0008, 0x00C8), 8 };      // Valid write (200%)
        case 10:  return { BuildRequestFrame(0x11, 0x03, 0x0008, 0x0001), 5 };      // Normal read
        default: return { BuildRequestFrame(0x11, 0x03, 0x0005, 0x0005), 5 };      // fallback
    }
}


// Function to print the data buffer
void printDataBuffer() {
    cout << "\n==== Logged Data Buffer ====\n";

    if (dataBuffer.empty()) {
        cout << "Buffer is empty.\n";
        cout << "============================\n";
        return;
    }

    time_t lastTime = 0;
    for (const auto& s : dataBuffer) {
        time_t t = chrono::system_clock::to_time_t(s.timestamp);

        // if this sample belongs to a new snapshot, print a header
        if (t != lastTime) {
            tm local_tm{};
            localtime_s(&local_tm, &t);
            cout << "\nUpdated Registers @ " << put_time(&local_tm, "%H:%M:%S") << "\n";
            lastTime = t;
        }

        // print register info
        cout << "  Reg " << s.regAddr;
        if (s.regAddr < registerMap.size()) {
            cout << " (" << registerMap[s.regAddr].name << ")";
        }
        cout << ": " << s.value;

        if (s.regAddr < registerMap.size()) {
            cout << " " << registerMap[s.regAddr].unit;
        }
        cout << endl;
    }

    cout << "============================\n";
}



int main() {
    string apiKey = "NjhhZWIwNDU1ZDdmMzg3MzNiMTQ5Yjg2OjY4YWViMDQ1NWQ3ZjM4NzMzYjE0OWI3Yw";
    string response, jsonFrame;
    vector<uint8_t> responseFrame;

    const size_t totalCases = 11;
    size_t caseIndex = 0;

    auto windowStart = chrono::steady_clock::now();

    while (true) {
        TestCase tc = getTestCase(caseIndex);
        logInfo("Running test case " + to_string(caseIndex+1));

        jsonFrame = frameToJson(tc.frame);

        if (tc.frame[1] == 0x03)
            response = readfromInverter(jsonFrame, apiKey);
        else
            response = writetoInverter(jsonFrame, apiKey);

        responseFrame = jsontoFrame(response);
        decodeResponseFrame(responseFrame, tc.startAddr); 

        caseIndex = (caseIndex + 1) % totalCases;

        auto now = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::seconds>(now - windowStart).count() >= 30) {
            logInfo("30-second window complete. Dumping buffer...");
            printDataBuffer();   // print all samples collected
            dataBuffer.clear();  // flush
            windowStart = now;   // restart timer
        }
        cout << "----------------------------------------\n";
        std::this_thread::sleep_for(std::chrono::seconds(5)); // poll every 5s
    }

    return 0;
}
