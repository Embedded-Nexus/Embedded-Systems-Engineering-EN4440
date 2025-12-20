#include "Uploader.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include "Delta16Compressor.h"   // Delta16VarCompressor + TimeSeriesCompressor
#include "SecurityStub.h"
#include "Aggregation.h"

EcoWattUploader::EcoWattUploader(const String& url, const String& key,
                                 unsigned long ms, size_t csz, int mr)
: baseUrl(url), apiKey(key), interval(ms), chunkSize(csz), maxRetries(mr) {}

String EcoWattUploader::deviceId() const {
  uint32_t id = ESP.getChipId();
  char buf[16]; sprintf(buf,"ESP%06X", id);
  return String(buf);
}

String EcoWattUploader::b64(const std::vector<uint8_t>& in){
  static const char* T="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  String out; out.reserve(((in.size()+2)/3)*4);
  size_t i=0;
  while (i+2<in.size()){
    uint32_t n=(in[i]<<16)|(in[i+1]<<8)|in[i+2];
    out += T[(n>>18)&63]; out += T[(n>>12)&63]; out += T[(n>>6)&63]; out += T[n&63];
    i+=3;
  }
  if (i+1==in.size()){
    uint32_t n=(in[i]<<16);
    out += T[(n>>18)&63]; out += T[(n>>12)&63]; out += "==";
  } else if (i+2==in.size()){
    uint32_t n=(in[i]<<16)|(in[i+1]<<8);
    out += T[(n>>18)&63]; out += T[(n>>12)&63]; out += T[(n>>6)&63]; out += '=';
  }
  return out;
}

bool EcoWattUploader::postChunk(uint32_t seqNo, uint64_t tStart, uint64_t tEnd,
                                const String& algo, int idx, int count,
                                const String& dataB64, const String& mac) {
  WiFiClient client; HTTPClient http;
  String url = baseUrl + "/api/uplink";
  http.begin(client, url);
  http.addHeader("Content-Type","application/json");
  http.addHeader("Authorization", apiKey);
  String json;
  json.reserve(256 + dataB64.length());
  json += "{\"deviceId\":\"" + deviceId() + "\",\"seqNo\":" + String(seqNo);
  json += ",\"tStart\":" + String((uint32_t)tStart) + ",\"tEnd\":" + String((uint32_t)tEnd);
  json += ",\"algo\":\"" + algo + "\",\"chunkIndex\":" + String(idx) + ",\"chunkCount\":" + String(count);
  json += ",\"data\":\"" + dataB64 + "\",\"mac\":\"" + mac + "\"}";

  int attempt=0; int backoff=400;
  while (attempt<=maxRetries){
    int code = http.POST(json);
    String body = (code>0)? http.getString(): String("");
    if (code==200 && body.indexOf("ack") >= 0) {http.end(); return true; }
    delay(backoff); backoff*=2; attempt++;
  }
  http.end(); return false;
}

void EcoWattUploader::forceUpload(SampleBuffer& buf){
  auto samples = buf.popAll();
  if (samples.empty()) return;

  uint64_t tStart = samples.front().timestamp;
  uint64_t tEnd   = samples.back().timestamp;

  // --- Aggregation ---
  auto stats = Aggregation::minAvgMax(samples);
  Serial.println("[AGG] Per-register min/avg/max for this window:");
  for (auto& r : stats) {
    Serial.printf("  Reg %u -> min=%u avg=%u max=%u (n=%u)\n",
      r.reg, r.minv, r.avgv, r.maxv, (unsigned)r.count);
  }

  auto values = SampleBuffer::exportValues(samples);
  const int REGS = 10;

  // --- Benchmarks ---
  auto bDelta = Delta16VarCompressor::benchmark(values);
  auto bTS    = TimeSeriesCompressor::benchmark(values, REGS);

  BenchResult best;
  std::vector<uint8_t> compressed;
  const char* algo = nullptr;

  if (bTS.lossless && bTS.compBytes < bDelta.compBytes){
    best = bTS;
    compressed = TimeSeriesCompressor::compress(values, REGS);
    algo = TimeSeriesCompressor::name();
  } else {
    best = bDelta;
    compressed = Delta16VarCompressor::compress(values);
    algo = Delta16VarCompressor::name();
  }

  // --- Benchmark report ---
  Serial.println("=== Compression Benchmark Report ===");
  Serial.printf("a. Compression Method Used: %s\n", best.mode);
  Serial.printf("b. Number of Samples: %u\n", best.samples);
  Serial.printf("c. Original Payload Size: %u bytes\n", best.origBytes);
  Serial.printf("d. Compressed Payload Size: %u bytes\n", best.compBytes);
  Serial.printf("e. Compression Ratio: %.2fx\n", 
                best.compBytes ? (float)best.origBytes / best.compBytes : 0.0f);
  Serial.printf("f. CPU Time: compress=%lu us, decompress=%lu us\n", 
                best.tCompressUs, best.tDecompressUs);
  Serial.printf("g. Lossless Recovery Verification: %s\n", 
                best.lossless ? "PASS" : "FAIL");

  // --- Encrypt + MAC stub ---
  auto cipher = SecurityStub::encrypt(compressed);
  auto mac = SecurityStub::mac(cipher);

  // --- Chunk & send ---
  size_t n = cipher.size();
  int chunks = (int)((n + chunkSize - 1) / chunkSize);
  if (chunks==0) chunks=1;

  bool ackAll = true;
  for (int i=0;i<chunks;i++){
    size_t off = i * chunkSize;
    size_t len = min(chunkSize, n-off);
    std::vector<uint8_t> piece(cipher.begin()+off, cipher.begin()+off+len);
    String dataB64 = b64(piece);
    bool ok = postChunk(seq, tStart, tEnd, String(algo), i, chunks, dataB64, mac);
    if (!ok){ 
      Serial.printf("[UP] chunk %d/%d FAIL (no ACK)\n", i+1, chunks); 
      ackAll = false;
    } else {
      Serial.printf("[UP] chunk %d/%d ACK received\n", i+1, chunks);
    }
  }

  Serial.printf("[UP] seq=%lu complete, algo=%s, ACK=%s\n",
    (unsigned long)seq, algo, ackAll ? "YES" : "NO");

  seq++;
}


void EcoWattUploader::periodicUpload(SampleBuffer& buf){
  if (millis() - lastTick >= interval){
    lastTick = millis();
    forceUpload(buf);
  }
}
