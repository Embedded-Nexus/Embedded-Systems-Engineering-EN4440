#include "Delta16Compressor.h"

// ---- utils ----
static inline void put_u16_be(std::vector<uint8_t>& o, uint16_t v){
  o.push_back((uint8_t)((v>>8)&0xFF));
  o.push_back((uint8_t)(v&0xFF));
}
static inline uint16_t get_u16_be(const std::vector<uint8_t>& b, size_t i){
  return (uint16_t)(((b[i]&0xFF)<<8) | (b[i+1]&0xFF));
}

//Delta16VarCompressor
static inline uint32_t zigzag32(int32_t x){ return (x<<1) ^ (x>>31); }
static inline int32_t unzigzag32(uint32_t x){ return (x>>1) ^ -int32_t(x&1); }

static void put_varu(std::vector<uint8_t>& o, uint32_t v){
  while (v >= 0x80){ o.push_back((uint8_t)(0x80 | (v & 0x7F))); v >>= 7; }
  o.push_back((uint8_t)v);
}
static uint32_t get_varu(const std::vector<uint8_t>& b, size_t& i){
  uint32_t v=0; int shift=0;
  while (i<b.size()){
    uint8_t byte = b[i++];
    v |= (uint32_t)(byte & 0x7F) << shift;
    if ((byte & 0x80) == 0) break;
    shift += 7;
  }
  return v;
}

std::vector<uint8_t> Delta16VarCompressor::compress(const std::vector<uint16_t>& values){
  std::vector<uint8_t> out;
  if (values.empty()) return out;
  out.reserve(values.size());
  put_u16_be(out, values[0]);
  for (size_t i=1;i<values.size();++i){
    int32_t d = (int32_t)values[i] - (int32_t)values[i-1];
    put_varu(out, zigzag32(d));
  }
  return out;
}

std::vector<uint16_t> Delta16VarCompressor::decompress(const std::vector<uint8_t>& blob){
  std::vector<uint16_t> out;
  if (blob.size() < 2) return out;
  out.reserve(blob.size());
  uint16_t prev = get_u16_be(blob,0);
  out.push_back(prev);
  size_t i = 2;
  while (i < blob.size()){
    uint32_t zz = get_varu(blob, i);
    int32_t d = unzigzag32(zz);
    prev = (uint16_t)((prev + d) & 0xFFFF);
    out.push_back(prev);
  }
  return out;
}

BenchResult Delta16VarCompressor::benchmark(const std::vector<uint16_t>& values){
  BenchResult br{}; br.samples = values.size(); br.origBytes = values.size()*2; br.mode = name();
  unsigned long t0 = micros();
  auto c = compress(values);
  unsigned long t1 = micros();
  auto r = decompress(c);
  unsigned long t2 = micros();
  br.compBytes = c.size();
  br.tCompressUs = t1 - t0;
  br.tDecompressUs = t2 - t1;
  br.lossless = (r == values);
  return br;
}

// TimeSeriesCompressor
static inline int8_t clamp_s4(int32_t d){
  if (d < -8 || d > 7) return 127; // mark as "needs absolute"
  return (int8_t)d;
}
static inline uint8_t pack_s4(int8_t v){ return (uint8_t)(v & 0x0F); }
static inline int8_t unpack_s4(uint8_t nib){
  return (nib & 0x8) ? (int8_t)(nib | 0xF0) : (int8_t)(nib & 0x0F);
}

std::vector<uint8_t> TimeSeriesCompressor::compress(const std::vector<uint16_t>& values, int regs){
  std::vector<uint8_t> out;
  if (regs <= 0 || values.empty()) return out;
  if (values.size() % (size_t)regs != 0){
    // Not a perfect multiple of frame width — fall back to delta16v
    return Delta16VarCompressor::compress(values);
  }
  size_t frames = values.size() / (size_t)regs;
  out.reserve(values.size());

  // first frame absolute
  for (int j=0;j<regs;++j) put_u16_be(out, values[j]);

  // subsequent frames
  for (size_t f=1; f<frames; ++f){
    const uint16_t* prev = &values[(f-1)*regs];
    const uint16_t* curr = &values[f*regs];
    uint16_t mask = 0;
    uint8_t nibbles[32];

    for (int j=0;j<regs;++j){
      int8_t s4 = clamp_s4((int32_t)curr[j] - (int32_t)prev[j]);
      if (s4 == 127){ mask |= (1u<<j); nibbles[j] = 0; }
      else          { nibbles[j] = pack_s4(s4); }
    }

    // mask (LE)
    out.push_back((uint8_t)(mask & 0xFF));
    out.push_back((uint8_t)((mask >> 8) & 0xFF));

    // packed nibbles (fixed regs/2 bytes)
    for (int j=0;j<regs; j+=2){
      uint8_t b = (uint8_t)((nibbles[j] << 4) | ((j+1<regs)? (nibbles[j+1] & 0x0F) : 0));
      out.push_back(b);
    }

    // absolutes for flagged regs
    for (int j=0;j<regs;++j)
      if (mask & (1u<<j)) put_u16_be(out, curr[j]);
  }
  return out;
}

std::vector<uint16_t> TimeSeriesCompressor::decompress(const std::vector<uint8_t>& blob, int regs){
  std::vector<uint16_t> out;
  if (regs <= 0 || blob.size() < (size_t)(regs*2)) return out;

  out.reserve(blob.size());
  size_t i=0;

  // first frame absolute
  std::vector<uint16_t> prev(regs);
  for (int j=0;j<regs;++j){
    prev[j] = get_u16_be(blob, i); i+=2;
    out.push_back(prev[j]);
  }

  // frames
  while (i < blob.size()){
    size_t need = 2 + (size_t)((regs+1)/2);
    if (i + need > blob.size()) break; // partial frame at end

    uint16_t mask = (uint16_t)(blob[i] | (blob[i+1] << 8)); i+=2;

    uint8_t nb = (uint8_t)((regs + 1) / 2);
    uint8_t packed[32];
    for (int k=0;k<nb;++k) packed[k] = blob[i++];

    std::vector<uint16_t> curr = prev;
    for (int j=0;j<regs;++j){
      uint8_t byte = packed[j/2];
      uint8_t nib  = (j%2==0) ? ((byte>>4)&0x0F) : (byte & 0x0F);
      if (mask & (1u<<j)){
        if (i+1 >= blob.size()) return out; // malformed guard
        curr[j] = get_u16_be(blob, i); i+=2;
      } else {
        curr[j] = (uint16_t)((curr[j] + unpack_s4(nib)) & 0xFFFF);
      }
    }
    for (int j=0;j<regs;++j){ out.push_back(curr[j]); prev[j]=curr[j]; }
  }
  return out;
}

BenchResult TimeSeriesCompressor::benchmark(const std::vector<uint16_t>& values, int regs){
  BenchResult br{}; br.samples = values.size(); br.origBytes = values.size()*2; br.mode = name();
  unsigned long t0 = micros();
  auto c = compress(values, regs);
  unsigned long t1 = micros();
  auto r = decompress(c, regs);
  unsigned long t2 = micros();
  br.compBytes = c.size();
  br.tCompressUs = t1 - t0;
  br.tDecompressUs = t2 - t1;
  br.lossless = (r == values);
  return br;
}
