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


# Milestone 3 – Local Buffering, Compression, and Upload Cycle

**EcoWatt Embedded Device**

---

## 1. Objective

The objective of **Milestone 3** is to extend the EcoWatt Device with **local buffering, lightweight compression, and a periodic upload cycle** so that acquired inverter data can be efficiently transmitted to the EcoWatt Cloud while respecting payload constraints.

This milestone demonstrates:

* Reliable local buffering across upload windows
* Lossless compression with benchmarking
* Aggregation for payload control
* Robust uplink packetization with retry logic
* End-to-end acquisition → compression → upload workflow

---

## 2. System Overview

```
Inverter SIM
   |
   |  (Modbus RTU via Cloud API)
   v
EcoWatt Device
 ├─ Acquisition (5 s)
 ├─ Local Buffer
 ├─ Compression + Aggregation
 ├─ Packetizer + Retry Logic
   |
   |  (HTTP JSON Upload)
   v
EcoWatt Cloud API
```

For demonstration purposes, the **15-minute upload interval is simulated using a 15-second window**, as allowed by the milestone specification.

---

## 3. Scope Implementation Mapping

---

## Part 1: Buffer Implementation 

### 3.1 Local Buffer Design

A **modular local buffer** is implemented to store all samples acquired during the upload interval without data loss.

Key properties:

* Fixed capacity with overflow handling
* Decoupled from acquisition and transport logic
* FIFO behavior with snapshot extraction
* Event callbacks for buffer health monitoring

Implemented in:

* `SampleBuffer` module (`Buffer.cpp`)

Stored data structure:

* Timestamp
* Register address
* Quantized sample value (uint16)

This design ensures **safe accumulation of samples across upload cycles** and is suitable for MCU memory constraints.

---

### 3.2 Data Integrity

* Overflow events are detected and logged
* Buffer watermarks provide early warning of capacity pressure
* Samples are only removed after successful upload finalization

This satisfies the **data integrity requirement** of the rubric.

---

## Part 2: Compression Algorithm and Benchmarking 

---

### 4.1 Compression Methods Implemented

At least one lightweight compression technique is required. This implementation includes **two**:

1. **Delta-16 Variable Length Compression**

   * First sample stored absolute
   * Subsequent samples stored as delta values
   * Zig-zag + variable-length encoding

2. **Time-Series Frame Compression**

   * Frame-based compression across multiple registers
   * Small deltas packed into 4-bit values
   * Absolute fallback for large deltas

Implemented in:

* `Delta16VarCompressor`
* `TimeSeriesCompressor`

---

### 4.2 Benchmarking Methodology

Compression is benchmarked **on real inverter data** collected from the Inverter SIM.

Measured fields:

* Compression method used
* Number of samples
* Original payload size
* Compressed payload size
* Compression ratio
* CPU time (microseconds)
* Lossless recovery verification

Benchmarking is executed at runtime and printed to the serial console during each upload window.

---

### 4.3 Example Benchmark Report (Runtime Output)

```
Compression Method Used: TimeSeries
Number of Samples: 300
Original Payload Size: 600 bytes
Compressed Payload Size: 148 bytes
Compression Ratio: 4.05x
CPU Time: compress=312 us, decompress=227 us
Lossless Recovery Verification: PASS
```

---

### 4.4 Lossless Verification

All compressed payloads are:

* Decompressed immediately after compression
* Byte-compared with original samples

Uploads proceed **only if lossless verification passes**, satisfying the milestone requirement.

---

### 4.5 Aggregation (Min / Avg / Max)

To support payload caps, **aggregation is implemented** per register:

* Minimum
* Average
* Maximum
* Sample count

Implemented in:

* `Aggregation` module

Aggregation results are logged and available for future cloud-side analytics, even though raw samples are still uploaded as required.

---

## Part 3: Uplink Packetizer and Upload Cycle 

---

### 5.1 Upload Cycle

The upload cycle follows the required workflow:

```
Upload Tick →
  Finalize buffer →
  Aggregate →
  Compress →
  Encrypt (stub) →
  Chunk →
  Upload →
  Await ACK
```

* Upload interval: **15 seconds (demo mode)**
* Each upload corresponds to one finalized buffer window

---

### 5.2 Packetizer Design

The packetizer:

* Encodes compressed data in Base64
* Splits payload into chunks
* Includes metadata (device ID, sequence number, timestamps)
* Retries failed uploads with exponential backoff

Implemented in:

* `EcoWattUploader`

Retry logic:

* Configurable max retries
* ACK-based success detection
* Partial failure reporting

---

### 5.3 Encryption Stub

A placeholder encryption + MAC interface is implemented:

* Compression output is passed through an encryption stub
* MAC field is generated and transmitted
* Interface is future-ready for Milestone 4 security integration

---

### 5.4 Cloud API Implementation

A custom **EcoWatt Cloud API endpoint** is implemented to:

* Accept uplink payloads
* Validate request format
* Return ACK responses

Both **client and server sides** are implemented as required.

---

## 6. Evaluation Rubric Mapping

| Criterion                       | Coverage                             |
| ------------------------------- | ------------------------------------ |
| Buffering & data integrity      |  Modular buffer + overflow handling |
| Compression & benchmarking      |  Two methods + runtime benchmarks   |
| Packetizer & upload robustness  |  Chunking + retry + ACK             |
| Cloud API implementation        |  Client + server implemented        |
| Video clarity & demo            |  Full live workflow                 |
| Code modularity & MCU readiness |  Clean separation of concerns       |

---

## 7. MCU Readiness

The design is optimized for embedded deployment:

* Fixed-size buffers
* Minimal dynamic allocation
* Simple data structures
* Clear separation of acquisition, buffering, compression, and transport

---

## 8. Deliverables Summary

✔ Source code (buffer, compression, uploader)
✔ Cloud API endpoint
✔ Compression benchmark report (runtime)
✔ Demonstration video
✔ GitHub repository

---

## 9. Conclusion

Milestone 3 successfully implements **local buffering, lossless compression, aggregation, and a robust upload cycle**, completing the EcoWatt Device’s data pipeline from acquisition to cloud ingestion and preparing the system for secure communication and remote configuration in future milestones.

