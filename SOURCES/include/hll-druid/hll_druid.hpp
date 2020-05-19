#ifndef _HLL_DRUID_H_
#define _HLL_DRUID_H_

#include <stdexcept>
#include <stdint.h>
#include <cstring>
#include <iostream>
#include <math.h>

#  if defined(__APPLE__)
#    include <libkern/OSByteOrder.h>
#    define __bswap_32 OSSwapInt32
#  else
#    include <byteswap.h>
#  endif

#include "../hll_utils.hpp"
#include "murmur3_hash.hpp"

namespace druid
{
static const uint8_t NUM_HEADER_BYTES = 7;
static const uint8_t BITS_PER_BUCKET = 4;
static const uint8_t BITS_PER_HASH = 128;
static const uint8_t BITS_FOR_BUCKETS = 11;

// number of set registers after which serialize to dense
static const uint8_t DENSE_THRESHOLD = 128;

static const uint16_t NUM_BUCKETS = 1 << BITS_FOR_BUCKETS;     // 2048 buckets
static const uint16_t NUM_BYTES_FOR_BUCKETS = NUM_BUCKETS / 2; // 1024 bytes
static const uint16_t BUCKET_MASK = 0x7ff;                     // used for deciding which bucket to use from hash

static constexpr const double TWO_TO_THE_SIXTY_FOUR = pow(2, 64);
static constexpr const double ALPHA = 0.7213 / (1 + 1.079 / NUM_BUCKETS);

static constexpr const double LOW_CORRECTION_THRESHOLD = (5 * NUM_BUCKETS) / 2.0;
static constexpr const double HIGH_CORRECTION_THRESHOLD = TWO_TO_THE_SIXTY_FOUR / 30.0;
static constexpr const double CORRECTION_PARAMETER = ALPHA * NUM_BUCKETS * NUM_BUCKETS;

// maximum value we can store in a nibble - cutoff for overflow register
static constexpr const unsigned RANGE = (1 << BITS_PER_BUCKET) - 1;

/*
    * Header:
    * Byte 0: version
    * Byte 1: registerOffset
    * Byte 2-3: numNonZeroRegisters
    * Byte 4: maxOverflowValue
    * Byte 5-6: maxOverflowRegister
    * payload:
    * as sparse: N * (uint16_t <registerId> uint8_t <upperNibble | lowerNibble>)
    *     where N is numNonZeroRegisters && N * 3 + sizeof(header)[7] == <length>
    * or payload as dense: 1024 * uint8_t <upperNibble | lowerNibble>
    *     where <length> = 1031
    */
struct Header {
  uint8_t version;
  uint8_t registerOffset;
  uint16_t numNonZeroRegisters;
  uint8_t maxOverflowValue;
  uint16_t maxOverflowRegister;
} __packed__;

union Hash128 {
  uint64_t longs[2];
  uint16_t shorts[8];
} __packed__;

class HllDruid
{

private:
  const uint8_t *payload;
  int length;

public:
  static HllDruid wrapRawBuffer(uint8_t *payload, size_t length)
  {
    if (length != NUM_BYTES_FOR_BUCKETS + NUM_HEADER_BYTES)
    {
      throw std::runtime_error("buffer is not of " + std::to_string(NUM_BYTES_FOR_BUCKETS + NUM_HEADER_BYTES) + " bytes");
    }
    return HllDruid(payload, length);
  }

  template <typename T>
  void add(T value)
  {
    std::string valStr = std::to_string(value);
    Hash128 out = {0};
    MurmurHash3_x64_128(valStr.c_str(), valStr.length(), 0, &out);
    add(out);
  }

  void add(const Hash128 &hash);
  void fold(const uint8_t *otherPayload, size_t length) { fold(HllDruid(otherPayload, length)); }
  void fold(const HllDruid &other);
  void serialize(uint8_t *outBuffer, size_t &outlength);
  size_t getSerializedBufferSize();

  void reset()
  {
    std::memset(this->mutablePayload(), 0, NUM_BYTES_FOR_BUCKETS + NUM_HEADER_BYTES);
    this->setVersion(1);
  }
  uint64_t approximateCountDistinct() { return this->estimateCardinality(); }

  uint8_t getVersion() const { return header().version; }
  uint8_t getRegisterOffset() const { return header().registerOffset; }
  uint16_t getNumNonZeroRegisters() const { return bswap_16(header().numNonZeroRegisters); }
  uint8_t getMaxOverFlowValue() const { return header().maxOverflowValue; }
  uint16_t getMaxOverFlowRegister() const { return bswap_16(header().maxOverflowRegister); }


private:
  HllDruid(const uint8_t *payload, size_t length) : payload(payload), length(length) {}

  uint8_t* mutablePayload() { return const_cast<uint8_t *>(payload); }
  Header& mutableHeader() { return *(reinterpret_cast<Header*>(mutablePayload())); }
  const Header& header() const { return *(reinterpret_cast<const Header*>(payload)); }

  uint8_t getPayloadBytePosition() const { return NUM_HEADER_BYTES; }

  void setVersion(uint8_t version) { mutableHeader().version = version; }
  void setRegisterOffset(uint8_t registerOffset) { mutableHeader().registerOffset = registerOffset; }
  void setNumNonZeroRegisters(uint16_t numNonZero) { mutableHeader().numNonZeroRegisters = bswap_16(numNonZero); }
  void setMaxOverflowValue(uint8_t maxOverflowValue) { mutableHeader().maxOverflowValue = maxOverflowValue; }
  void setMaxOverflowRegister(uint16_t maxOverflowRegister) { mutableHeader().maxOverflowRegister = bswap_16(maxOverflowRegister); }

  bool isSparse() const { return this->length != (NUM_BYTES_FOR_BUCKETS + NUM_HEADER_BYTES); }
  uint16_t decrementBuckets();
  uint16_t addNibbleRegister(uint16_t bucket, uint8_t positionOf1);
  void addRegister(uint16_t bucket, uint8_t positionOf1);
  uint64_t estimateCardinality() const;
};

} // namespace druid

#endif
