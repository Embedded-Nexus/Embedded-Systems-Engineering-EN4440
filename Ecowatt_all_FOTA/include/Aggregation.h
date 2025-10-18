#pragma once
#include "Buffer.h"
#include <vector>

struct RegStats {
  uint16_t reg;
  uint16_t minv;
  uint16_t maxv;
  uint16_t avgv;
  size_t count;
};

namespace Aggregation {
  // Compute min/avg/max for each register in a batch of samples
  std::vector<RegStats> minAvgMax(const std::vector<Sample>& samples);
}
