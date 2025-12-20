#include "security_layer.h"

// ---------- Constants ----------
#define NONCE_LEN 12
#define TAG_LEN   8
#define SEQ_LEN   4

static const uint8_t PSK[] = {
  0x23, 0xAF, 0x77, 0x1D, 0x9B, 0x0F, 0xA5, 0x44,
  0xC1, 0xE9, 0x56, 0x72, 0xAA, 0xDE, 0x19, 0xBB
};
static const size_t PSK_LEN = sizeof(PSK);

// ---------- State ----------
static uint32_t seqCounter = 1;
static uint32_t lastSeqReceived = 0;

// ---------- Helpers ----------
static uint32_t rotl32(uint32_t x, uint8_t r) { return (x << r) | (x >> (32 - r)); }

static uint32_t xorshift32(uint32_t &s) {
  s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s;
}

static uint32_t deriveSeed(const uint8_t* psk, size_t pskLen,
                           const uint8_t* nonce, size_t nonceLen,
                           uint32_t seq) {
  uint32_t acc = 2166136261u;
  for (size_t i=0;i<pskLen;++i){ acc ^= psk[i]; acc *= 16777619u; }
  for (size_t i=0;i<nonceLen;++i){ acc ^= nonce[i]; acc *= 16777619u; }
  acc ^= seq; acc *= 16777619u;
  acc ^= rotl32(acc,13); acc ^= acc>>7; acc ^= rotl32(acc,17);
  if(acc==0) acc=1;
  return acc;
}

static uint64_t fnv1a64_keyed(const uint8_t* key,size_t keyLen,
                              const uint8_t* header,size_t headerLen,
                              const uint8_t* data,size_t dataLen){
  const uint64_t OFFSET=1469598103934665603ull;
  const uint64_t PRIME=1099511628211ull;
  uint64_t h=OFFSET;
  for(size_t i=0;i<keyLen;++i){ h^=key[i]; h*=PRIME; }
  for(size_t i=0;i<headerLen;++i){ h^=header[i]; h*=PRIME; }
  for(size_t i=0;i<dataLen;++i){ h^=data[i]; h*=PRIME; }
  return h;
}

static void xorStreamTransform(const uint8_t* in,uint8_t* out,size_t len,
                               const uint8_t* psk,size_t pskLen,
                               const uint8_t* nonce,size_t nonceLen,
                               uint32_t seq){
  uint32_t s=deriveSeed(psk,pskLen,nonce,nonceLen,seq);
  for(size_t i=0;i<len;++i){
    if((i&3)==0) xorshift32(s);
    uint8_t ks=(uint8_t)((s>>((i&3)*8))&0xFF);
    out[i]=in[i]^ks;
  }
}

static void buildHeader(uint8_t* out,const uint8_t* nonce,uint32_t seq){
  memcpy(out,nonce,NONCE_LEN);
  out[NONCE_LEN+0]=(uint8_t)(seq&0xFF);
  out[NONCE_LEN+1]=(uint8_t)((seq>>8)&0xFF);
  out[NONCE_LEN+2]=(uint8_t)((seq>>16)&0xFF);
  out[NONCE_LEN+3]=(uint8_t)((seq>>24)&0xFF);
}

// ======================================================================
//                            ENCRYPTION
// ======================================================================
std::vector<uint8_t> encryptBuffer(const std::vector<uint8_t>& plain){
  std::vector<uint8_t> nonce(NONCE_LEN);
  for(size_t i=0;i<NONCE_LEN;i++) nonce[i]=(uint8_t)random(0,256);

  std::vector<uint8_t> cipher(plain.size());
  xorStreamTransform(plain.data(),cipher.data(),plain.size(),PSK,PSK_LEN,nonce.data(),NONCE_LEN,seqCounter);

  uint8_t header[NONCE_LEN+SEQ_LEN];
  buildHeader(header,nonce.data(),seqCounter);
  uint64_t tag = fnv1a64_keyed(PSK,PSK_LEN,header,sizeof(header),cipher.data(),cipher.size());

  std::vector<uint8_t> packet;
  packet.reserve(SEQ_LEN + NONCE_LEN + cipher.size() + TAG_LEN);

  // sequence number (LE)
  packet.push_back((uint8_t)(seqCounter & 0xFF));
  packet.push_back((uint8_t)((seqCounter >> 8) & 0xFF));
  packet.push_back((uint8_t)((seqCounter >> 16) & 0xFF));
  packet.push_back((uint8_t)((seqCounter >> 24) & 0xFF));

  // nonce + cipher
  packet.insert(packet.end(), nonce.begin(), nonce.end());
  packet.insert(packet.end(), cipher.begin(), cipher.end());

  // tag
  for(int i=0;i<8;i++)
    packet.push_back((uint8_t)((tag >> (i*8)) & 0xFF));

  seqCounter++;
  return packet;
}

// ======================================================================
//                            DECRYPTION
// ======================================================================
std::vector<uint8_t> decryptBuffer(const std::vector<uint8_t>& packet){
  std::vector<uint8_t> empty; // return if error

  if(packet.size() < SEQ_LEN + NONCE_LEN + TAG_LEN) return empty;

  uint32_t seqIn = packet[0] | (packet[1]<<8) | (packet[2]<<16) | (packet[3]<<24);
  const uint8_t* nonceIn = &packet[SEQ_LEN];
  size_t cipherLen = packet.size() - (SEQ_LEN + NONCE_LEN + TAG_LEN);
  const uint8_t* cipherIn = &packet[SEQ_LEN + NONCE_LEN];
  const uint8_t* tagPtr = &packet[SEQ_LEN + NONCE_LEN + cipherLen];

  uint64_t tagIn=0;
  for(int i=0;i<8;i++) tagIn |= ((uint64_t)tagPtr[i])<<(i*8);

  uint8_t header[NONCE_LEN+SEQ_LEN];
  buildHeader(header,nonceIn,seqIn);
  uint64_t expected = fnv1a64_keyed(PSK,PSK_LEN,header,sizeof(header),cipherIn,cipherLen);
  if(expected != tagIn) return empty;

  if(seqIn <= lastSeqReceived) return empty; // anti-replay

  std::vector<uint8_t> plain(cipherLen);
  xorStreamTransform(cipherIn,plain.data(),cipherLen,PSK,PSK_LEN,nonceIn,NONCE_LEN,seqIn);
  lastSeqReceived = seqIn;
  return plain;
}
