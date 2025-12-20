#pragma once
#include <Arduino.h>
#include <vector>
#include "Buffer.h"

class EcoWattUploader {
public:
  EcoWattUploader(const String& baseUrl, const String& apiKey,
                  unsigned long uploadIntervalMs=5000,
                  size_t chunkSize=600, int maxRetries=3);

  void periodicUpload(SampleBuffer& buf);
  void forceUpload(SampleBuffer& buf);

private:
  String baseUrl, apiKey;
  unsigned long interval, lastTick=0;
  size_t chunkSize; int maxRetries;
  uint32_t seq=1;

  String deviceId() const;
  bool postChunk(uint32_t seqNo, uint64_t tStart, uint64_t tEnd,
                 const String& algo, int idx, int count,
                 const String& b64, const String& mac);

  String b64(const std::vector<uint8_t>& bytes);
};
