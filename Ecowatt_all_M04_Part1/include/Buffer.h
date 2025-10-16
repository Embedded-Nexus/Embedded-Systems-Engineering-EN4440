#pragma once
#include <Arduino.h>
#include <vector>
#include <functional>
#include <cstdint>

struct Sample { uint32_t timestamp; uint16_t regAddr; uint16_t value; };

enum class BufferEvent { OVERFLOW, LOW_WATERMARK, HIGH_WATERMARK };
using BufferCallback = std::function<void(BufferEvent, size_t)>;

class SampleBuffer {
public:
  explicit SampleBuffer(size_t capacity = 1024);

  bool addSample(const Sample& s);
  size_t size() const;

  std::vector<Sample> popSamples(size_t N);
  std::vector<Sample> popAll();
  std::vector<Sample> peekSamples(size_t N) const;

  void clear();

  void setWatermarks(size_t low, size_t high);
  void setCallback(BufferCallback cb);

  static std::pair<int32_t,int32_t> minMaxForRegister(const std::vector<Sample>& samples, uint16_t regAddr);
  static float meanForRegister(const std::vector<Sample>& samples, uint16_t regAddr);
  static std::vector<uint16_t> exportValues(const std::vector<Sample>& samples);
  static String toJson(const std::vector<Sample>& samples);

private:
  size_t capacity;
  std::vector<Sample> buf;
  size_t lowWatermark=0, highWatermark=0;
  BufferCallback callback=nullptr;
  void notify(BufferEvent ev);
};
