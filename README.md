# Embedded-Systems-Engineering-EN4440
We are building a small embedded device called EcoWatt Device that pretends it is plugged into a real solar inverter

# Milestone 2 – Inverter SIM Integration and Basic Acquisition

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

# Milestone 3 – Local Buffering, Compression, and Upload Cycle

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

# Milestone 4 – Remote Configuration, Security Layer & FOTA

**EcoWatt Embedded Device (ESP8266)**

---

## 1. Objective

Milestone 4 enables **secure, remotely managed operation** of the EcoWatt Device.
The system now supports:

* Runtime remote configuration without reboot
* Cloud-driven command execution via Inverter SIM
* Lightweight MCU-friendly security (auth, integrity, confidentiality, anti-replay)
* Secure firmware-over-the-air (FOTA) with rollback protection

---

## 2. Complete Runtime Architecture

```
EcoWatt Cloud
 ├─ /data        (uplink payloads)
 ├─ /config      (runtime configuration)
 ├─ /commands    (queued commands)
 ├─ /firmware    (FOTA images)
 ├─ Logs & Audit
 |
 |  Secure JSON / Binary Payloads
 v
EcoWatt Device
 ├─ PollingManager
 ├─ ProtocolAdapter + FrameQueue
 ├─ Inverter SIM Interface
 ├─ TemporaryBuffer → Main Buffer
 ├─ Compression
 ├─ Security Layer (Encrypt + MAC + Anti-Replay)
 ├─ UploadManager
 ├─ Runtime Config Manager
 ├─ FirmwareUpdater + Rollback
 |
 v
Inverter SIM
```

---

## 3. Part 1 – Remote Configuration 

### 3.1 Supported Runtime Parameters

The EcoWatt Device supports **cloud-driven runtime updates** for:

* Polling interval
* Register read enable/disable bitmap
* Firmware version metadata
* Command enablement behavior

Configurations are fetched from the cloud **during the upload cycle**, not asynchronously, ensuring safe application boundaries.

---

### 3.2 Runtime Application (No Reboot)

Configuration updates:

* Are validated on receipt
* Are applied **after the next upload window**
* Do not interrupt polling, buffering, or command execution
* Do not require reboot or reflashing

This is implemented via:

* Global `RequestSIM` structure
* Deferred application in `UploadManager::handle()`
* Non-blocking update logic

---

### 3.3 Validation, Idempotency & Error Handling

* Invalid or unsafe values are rejected
* Duplicate configurations are ignored (idempotent behavior)
* Partial success is supported
* Detailed debug output explains applied vs rejected parameters

---

### 3.4 Persistence Across Power Cycles

Accepted configuration parameters are stored in non-volatile memory so that:

* Settings survive power loss
* Device resumes with last valid configuration

---

### 3.5 Cloud-Side Responsibilities

EcoWatt Cloud:

* Issues configuration updates
* Tracks application status
* Maintains timestamped logs of configuration history

---

## 4. Part 2 – Command Execution 

### 4.1 Command Execution Workflow

The implemented command pipeline strictly follows the required sequence:

```
Cloud queues command
↓
Next upload cycle
↓
EcoWatt Device fetches commands
↓
ProtocolAdapter builds Modbus frames
↓
Inverter SIM executes command
↓
Result returned to device
↓
ACK + execution report uploaded to cloud
```

---

### 4.2 Implementation Details

* Commands are parsed from cloud JSON
* Each command is converted into a temporary `RequestSIM`
* Modbus frames are built using `ProtocolAdapter`
* Frames are executed with retry logic (up to 3 attempts)
* CRC and Modbus exception handling are enforced
* Results are logged and acknowledged

---

### 4.3 Auditing and Reporting

Both device and cloud maintain:

* Command ID
* Target register
* Timestamp
* Execution result
* Failure reason (if any)

---

## 5. Part 3 – Security Layer 

### 5.1 Security Goals

The security layer is:

* Lightweight
* MCU-friendly
* Independent of heavyweight crypto libraries
* Deterministic and auditable

---

### 5.2 Authentication & Integrity

* Pre-Shared Key (PSK) embedded on device
* Keyed FNV-1a MAC over header + payload
* Invalid MACs are rejected immediately

---

### 5.3 Confidentiality

* Payloads encrypted using a stream-cipher–style XOR transform
* Keystream derived from PSK + nonce + sequence number
* Clean abstraction for future AES/HMAC upgrade

---

### 5.4 Anti-Replay Protection

* Every packet carries a monotonic sequence number
* Device rejects:

  * Replayed packets
  * Out-of-order packets

---

### 5.5 Secure Key Handling

* PSK stored locally on device
* Never transmitted
* Suitable for ESP8266-class constraints

---

## 6. Part 4 – FOTA Module 

### 6.1 Firmware Update Lifecycle

```
Upload cycle →
Check firmware version →
Download firmware chunks →
Verify integrity →
Mark update-in-progress →
Controlled reboot →
Verify boot →
Confirm success OR rollback
```

---

### 6.2 Chunked Download & Resume

* Firmware is downloaded incrementally
* Interrupted downloads can resume
* Progress is logged and reported

---

### 6.3 Verification

* Firmware authenticity and integrity verified before activation
* Invalid images never boot

---

### 6.4 Controlled Reboot

* Reboot only after successful verification
* Explicit reboot command issued
* Automatic reboot disabled during flashing

---

### 6.5 Rollback Mechanism

Rollback is implemented using **RTC memory**:

* Update-in-progress flag
* Boot counter (max 3 attempts)
* Failed update tracking
* Automatic fallback to previous firmware slot

This guarantees **device recovery from faulty firmware**.

---

### 6.6 Failure Demonstration

At least one demo includes:

* Simulated firmware failure
* Boot failure detection
* Automatic rollback
* Confirmation of restored firmware

---

## 7. Power & Reliability Instrumentation

A power estimator continuously tracks:

* CPU active time
* Wi-Fi usage
* Sleep time
* Estimated energy consumption per cycle

This ensures Milestone 4 remains viable for **low-power IoT deployment**.

---

## 8. Part 5 – Demonstration Video 

The Milestone 4 demo video (≤ 10 minutes) includes:

* Runtime configuration update with validation
* Command execution round-trip
* Secure transmission (encryption + MAC)
* Full FOTA update
* Simulated failure + rollback
* Presenter visible on camera

*(Presenter has not presented in previous milestone videos.)*

---

## 9. Evaluation Rubric Mapping

| Criterion                    | Coverage                                   |
| ---------------------------- | ------------------------------------------ |
| Runtime remote configuration |  Deferred, validated, persistent          |
| Command execution            |  Cloud → SIM → Cloud                      |
| Security implementation      |  Auth, integrity, encryption, anti-replay |
| FOTA + rollback              |  Chunked, verified, RTC rollback          |
| Video clarity                |  Live demos                               |
| Documentation quality        |  Complete & accurate                      |

---

## 10. Deliverables Summary

✔ Full embedded source code
✔ Cloud endpoints
✔ Runtime configuration support
✔ Secure command execution
✔ Security layer
✔ FOTA with rollback
✔ Documentation
✔ Demonstration video

# Milestone 5 – Fault Recovery, Power Optimization & Final Integration

---

## 1. Objective

The objective of **Milestone 5** is to finalize the EcoWatt system by integrating **power optimization**, **fault recovery**, and **end-to-end system validation**.
This milestone demonstrates that all features developed in Milestones 1–4 operate together reliably under both normal and fault conditions, while minimizing power consumption.

No new architecture was introduced in this milestone; instead, the **Milestone 4 codebase was extended and validated** to meet all Milestone 5 requirements.

---

## 2. Integrated System Overview

```
EcoWatt Cloud
 ├─ Data Upload API
 ├─ Configuration API
 ├─ Command Queue API
 ├─ Firmware API
 ├─ Logs & Audit
 |
 |  Secure JSON / Binary Payloads
 v
EcoWatt Device (ESP8266)
 ├─ Polling & Acquisition
 ├─ Local Buffers
 ├─ Compression & Aggregation
 ├─ Security Layer (Auth, Encrypt, Anti-Replay)
 ├─ Upload Manager
 ├─ Runtime Configuration Manager
 ├─ Command Execution Engine
 ├─ FOTA + Rollback
 ├─ Power Estimator
 ├─ Fault & Event Logger
 |
 v
Inverter SIM
```
---

## 3. Part 1 – Power Management & Measurement 

### 3.1 Power-Saving Mechanisms Implemented

The EcoWatt Device applies the following power-saving techniques:

* **Sleep / Idle Between Operations**

  * Polling and upload are timer-driven
  * Device remains idle between scheduled activities
  * `yield()` used to avoid busy waiting

* **Peripheral Gating**

  * Wi-Fi enabled only during upload windows
  * HTTP clients instantiated and destroyed per cycle
  * No persistent network connections

* **Task Consolidation**

  * Firmware update, upload, configuration, and command handling executed within a single upload window

---

### 3.2 Self-Energy-Use Reporting

A lightweight **power estimator** is implemented to measure:

* CPU time spent on compression
* CPU time spent on encryption/decryption
* Active processing duration per cycle

This provides a practical estimate of energy usage suitable for MCU-class devices.

---

### 3.3 Power Measurement Report

Power savings were evaluated by comparing:

* System behavior before optimization (continuous activity)
* Optimized behavior (interval-based polling and uploads)

A short power report summarizes:

* CPU active time per cycle
* Relative duty-cycle reduction
* Estimated energy savings

---

## 4. Part 2 – Fault Recovery 

### 4.1 Fault Handling Capabilities

The system robustly handles the following fault scenarios:

* Inverter SIM timeouts
* Malformed Modbus frames
* Corrupted or invalid JSON payloads
* Network outages during upload
* Buffer pressure / low-memory conditions
* FOTA verification failures

All faults are handled **gracefully**, without system crashes or data loss.

---

### 4.2 Local Event Logging

A local event log is maintained on the device, recording:

* Timestamp
* Fault type
* Recovery action taken

The log buffer is circular and bounded, ensuring reliability on MCU-class hardware.

---

### 4.3 Fault Injection Methodology

Faults were injected using **Postman**, as allowed by the milestone specification, by sending:

* Malformed JSON payloads
* Corrupted encrypted uploads
* Invalid configuration updates
* Interrupted firmware update metadata
* Simulated network failures

This approach simulates real-world production faults without modifying firmware.

---

### 4.4 Fault Injection Test Summary

| Fault Scenario           | Injection Method     | Observed Behavior     |
| ------------------------ | -------------------- | --------------------- |
| SIM timeout              | Dropped API response | Retry + continue      |
| Malformed config JSON    | Invalid body         | Rejected + logged     |
| Corrupted upload payload | Modified bytes       | MAC failure, rejected |
| Network outage           | Wi-Fi disabled       | Buffer retained       |
| FOTA hash mismatch       | Invalid firmware     | Update aborted        |
| FOTA boot failure        | Simulated failure    | Rollback executed     |

All tests resulted in **correct recovery behavior**.

---

## 5. Part 3 – Final Integration & System Testing 

### 5.1 Unified Firmware

A single firmware build integrates:

* Acquisition
* Buffering
* Compression
* Secure upload
* Remote configuration
* Command execution
* FOTA with rollback
* Power estimation
* Fault logging

This satisfies the **full integration requirement**.

---

### 5.2 End-to-End Workflow

```
Acquire →
Buffer →
Compress →
Encrypt →
Upload →
Fetch Config →
Apply Config →
Fetch Commands →
Execute →
ACK →
FOTA Check →
Sleep
```

This workflow was validated under both normal and fault conditions.

---

## 6. Part 4 – Live Demonstration 

The live demonstration verified all features using a checklist, including:

* Normal acquisition and upload
* Remote configuration updates
* Command execution round-trip
* Secure transmission (encrypted payloads)
* FOTA update success
* FOTA failure with rollback
* Network fault recovery

The presenter was visible on camera and followed the provided checklist.

---

## 7. Evaluation Rubric Mapping

| Criterion                         | Coverage                  |
| --------------------------------- | ------------------------- |
| Power savings & measurement       |  Implemented + reported  |
| End-to-end integration & recovery |  Fully demonstrated      |
| Video clarity & demo              |  Checklist-based         |
| Documentation quality             |  Complete & structured   |
| Code modularity & MCU readiness   |  Clean, unified firmware |

---

## 8. Deliverables Summary

✔ Full integrated firmware source code
✔ Power measurement report
✔ Fault injection test documentation
✔ Demonstration video
✔ GitHub repository

---




