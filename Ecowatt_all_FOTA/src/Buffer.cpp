#include "Buffer.h"
#include <ArduinoJson.h>

SampleBuffer::SampleBuffer(size_t cap): capacity(cap) { buf.reserve(min((size_t)128, cap)); }

bool SampleBuffer::addSample(const Sample& s) {
  if (buf.size() >= capacity) {
    notify(BufferEvent::OVERFLOW);
    if (!buf.empty()) buf.erase(buf.begin());
  }
  buf.push_back(s);
  if (lowWatermark && buf.size() <= lowWatermark) notify(BufferEvent::LOW_WATERMARK);
  if (highWatermark && buf.size() >= highWatermark) notify(BufferEvent::HIGH_WATERMARK);
  return true;
}
size_t SampleBuffer::size() const { return buf.size(); }

std::vector<Sample> SampleBuffer::popSamples(size_t N) {
  std::vector<Sample> out; if (N==0 || buf.empty()) return out;
  size_t n=min(N, buf.size()); out.reserve(n);
  for (size_t i=0;i<n;i++) out.push_back(buf[i]);
  buf.erase(buf.begin(), buf.begin()+n);
  return out;
}
std::vector<Sample> SampleBuffer::popAll() { auto out=buf; buf.clear(); return out; }

std::vector<Sample> SampleBuffer::peekSamples(size_t N) const {
  std::vector<Sample> out; if (N==0 || buf.empty()) return out;
  size_t n=min(N, buf.size()); out.reserve(n);
  for (size_t i=0;i<n;i++) out.push_back(buf[i]); return out;
}

void SampleBuffer::clear(){ buf.clear(); }
void SampleBuffer::setWatermarks(size_t low,size_t high){ lowWatermark=low; highWatermark=high; }
void SampleBuffer::setCallback(BufferCallback cb){ callback=cb; }
void SampleBuffer::notify(BufferEvent ev){ if (callback) callback(ev, buf.size()); }

std::pair<int32_t,int32_t> SampleBuffer::minMaxForRegister(const std::vector<Sample>& samples, uint16_t r) {
  int32_t mn=INT32_MAX,mx=INT32_MIN; bool f=false;
  for (auto&s:samples) if (s.regAddr==r){ f=true; mn=min<int32_t>(mn,s.value); mx=max<int32_t>(mx,s.value); }
  return f?std::make_pair(mn,mx):std::make_pair(0,0);
}
float SampleBuffer::meanForRegister(const std::vector<Sample>& samples, uint16_t r) {
  uint64_t sum=0; size_t c=0; for(auto&s:samples) if(s.regAddr==r){ sum+=s.value; c++; } return c? (float)sum/(float)c:0.0f;
}
std::vector<uint16_t> SampleBuffer::exportValues(const std::vector<Sample>& samples){
  std::vector<uint16_t> out; out.reserve(samples.size()); for(auto&s:samples) out.push_back(s.value); return out;
}
String SampleBuffer::toJson(const std::vector<Sample>& samples){
  DynamicJsonDocument doc(8192); JsonArray arr=doc.to<JsonArray>();
  for (auto&s:samples){ auto o=arr.createNestedObject(); o["t"]=s.timestamp; o["r"]=s.regAddr; o["v"]=s.value; }
  String out; serializeJson(doc,out); return out;
}
