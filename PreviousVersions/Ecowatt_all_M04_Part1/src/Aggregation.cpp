#include "Aggregation.h"
#include <unordered_map>
#include <limits>

std::vector<RegStats> Aggregation::minAvgMax(const std::vector<Sample>& samples) {
  struct Acc { uint32_t sum=0; uint16_t mn=UINT16_MAX, mx=0; size_t n=0; };
  std::unordered_map<uint16_t, Acc> acc;

  for (auto& s : samples) {
    auto& a = acc[s.regAddr];
    a.n++;
    a.sum += s.value;
    if (s.value < a.mn) a.mn = s.value;
    if (s.value > a.mx) a.mx = s.value;
  }

  std::vector<RegStats> out;
  out.reserve(acc.size());
  for (auto& kv : acc) {
    auto reg = kv.first;
    auto& a = kv.second;
    uint16_t avg = a.n ? (uint16_t)(a.sum / a.n) : 0;
    out.push_back(RegStats{reg, (a.mn == UINT16_MAX ? 0 : a.mn), a.mx, avg, a.n});

  }
  return out;
}
