# Embedded-Systems-Engineering-EN4440
We are building a small embedded device called EcoWatt Device that pretends it is plugged into a real solar inverter

# Milestone 2 – Inverter SIM Integration and Basic Acquisition

**EcoWatt Embedded Device**

---

## 1. Objective

The objective of **Milestone 2** is to integrate the EcoWatt Device with the **Inverter SIM** and establish **reliable round-trip communication** for reading and writing inverter registers.

This milestone demonstrates:

* Successful protocol adaptation between EcoWatt and the Inverter SIM
* Robust Modbus frame handling with error detection and recovery
* Periodic data acquisition and in-memory storage of inverter measurements

---

## 2. System Overview

The EcoWatt Device communicates with the Inverter SIM through a **cloud-based API**, encapsulating **Modbus RTU frames** inside JSON over HTTP.

```
EcoWatt Device (C++)
   |
   |  JSON + HTTP (libcurl)
   v
Inverter SIM Cloud API
   |
   |  Modbus RTU Frames
   v
Simulated Inverter Registers
```

---

## 3. Scope Implementation Mapping

---

## Part 1: Protocol Adapter Implementation 

### 3.1 Request Formatting

The protocol adapter constructs Modbus RTU frames according to the standard:

**Frame Format**

```
[ Slave Addr ][ Function Code ][ Start Addr ][ Reg Count / Value ][ CRC16 ]
```

Implemented in:

```cpp
vector<uint8_t> BuildRequestFrame(...)
```

Supported function codes:

* `0x03` – Read Holding Registers
* `0x06` – Write Single Register

Frames are converted to JSON for cloud transmission using:

```cpp
string frameToJson(...)
```

---

### 3.2 Response Parsing

Responses from the Inverter SIM are:

1. Converted from JSON to raw bytes
2. Validated using CRC-16
3. Decoded into scaled engineering values

Implemented in:

```cpp
vector<uint8_t> jsontoFrame(...)
ValidateResponseFrame(...)
decodeResponseFrame(...)
```

---

### 3.3 Error Detection and Recovery

The protocol adapter handles:

* CRC mismatches
* Empty or malformed frames
* Modbus exception responses
* Network/API failures

Error handling includes:

* Logging with timestamps
* Retry logic (up to 3 attempts)
* Graceful failure reporting

Implemented in:

```cpp
readfromInverter(...)
writetoInverter(...)
logError(...)
```

---

### 3.4 Retry Logic

Each read/write request:

* Retries up to **3 times**
* Introduces a delay between retries
* Logs each failed attempt

This meets the requirement for **transient failure recovery**.

---

## Part 2: Basic Data Acquisition 

---

### 4.1 Acquisition Scheduler

A basic acquisition scheduler is implemented using a polling loop:

* Polling interval: **5 seconds**
* Configurable test cases simulate acquisition tables
* Scheduler cycles continuously

Implemented in:

```cpp
while (true) {
    ...
    std::this_thread::sleep_for(std::chrono::seconds(5));
}
```

---

### 4.2 Polled Registers

The acquisition includes:

* Phase voltage
* Phase current
* Frequency
* PV voltage and current
* Inverter temperature
* Export power percentage

Registers are defined in a structured register map:

```cpp
vector<RegisterInfo> registerMap;
```

---

### 4.3 In-Memory Data Storage

Raw samples are stored in memory using a **modular data structure**, separate from protocol code.

Each sample contains:

* Timestamp
* Register address
* Scaled value

```cpp
struct Sample {
    timestamp;
    register address;
    value;
};
```

Samples are grouped into snapshots and stored in:

```cpp
vector<Sample> dataBuffer;
```

---

### 4.4 Write Operation

At least one write operation is performed:

* Writing export power percentage
* Validated via read-back

Implemented using:

```cpp
Function Code 0x06
```

---

## Part 3: Demonstration Video 

The demonstration video (2–4 minutes) includes:

* Walkthrough of the protocol adapter
* Explanation of Modbus message format
* Error handling and retry logic
* Acquisition scheduler walkthrough
* Live read and write round-trip demo
* Presenter visible on camera

*(Video submitted separately as per instructions)*

---

## 5. Test Scenarios

The system executes multiple test cases:

* Multi-register reads
* Single-register reads
* Write followed by validation read
* Boundary register access

Defined in:

```cpp
TestCase getTestCase(...)
```

---

## 6. Code Modularity and MCU Readiness

The code is designed for easy MCU porting:

* Protocol logic isolated from acquisition logic
* Clear data structures
* Minimal dynamic allocation
* No OS-specific dependencies in core logic

---

## 7. Build and Run Instructions

### Requirements

* C++17 compatible compiler
* libcurl

### Compile

```bash
g++ -std=c++17 main.cpp -lcurl -o ecowatt_m2
```

### Run

```bash
./ecowatt_m2
```

---

## 8. Deliverables Summary

✔ Source code (GitHub)
✔ Protocol adapter implementation
✔ Acquisition scheduler
✔ Test scenarios
✔ Demonstration video

---

## 9. Evaluation Rubric Mapping

| Criterion                | Coverage                    |
| ------------------------ | --------------------------- |
| Read & write operations  |  Implemented and validated |
| Protocol adapter clarity |  CRC, retries, exceptions  |
| Code modularity          |  MCU-ready design          |
| Video clarity            |  Live demo + walkthrough   |
| Documentation            |  Complete & structured     |

---

## 10. Conclusion

Milestone 2 successfully demonstrates **robust inverter communication**, **cloud-based Modbus integration**, and **basic data acquisition**, meeting all specified requirements and preparing the system for future milestones involving configuration updates and persistence.
