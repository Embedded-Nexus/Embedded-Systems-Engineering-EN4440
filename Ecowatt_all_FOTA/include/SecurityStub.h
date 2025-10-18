#pragma once
#include <vector>
#include <cstdint>
#include <Arduino.h>

namespace SecurityStub {
inline std::vector<uint8_t> encrypt(const std::vector<uint8_t>& in){ return in; }
inline String mac(const std::vector<uint8_t>& in){
  char buf[17]; size_t n=min<size_t>(in.size(),8);
  for(size_t i=0;i<n;i++) sprintf(&buf[i*2],"%02X",in[i]);
  buf[n*2]=0;
  return String("stubmac:")+String(buf);
}
}
