#include "hll-druid/hll_druid.hpp"
#include "../base_test.hpp"
#include "gtest/gtest.h"

#include <stdint.h>
#include <fstream>

using namespace std;
using namespace druid;

namespace
{

class HllDruidTest : public HllBaseTest {
 protected:
  std::ifstream data_file;

  HllDruidTest() : HllBaseTest("ids.dat"),
    data_file{getInputPath(), std::ifstream::in} {}

  virtual ~HllDruidTest() {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  virtual void SetUp() {
    data_file.seekg(0, data_file.beg);
    // ignore the first line of the data file; it contains a comment
    data_file.ignore(256, '\n');
  }

  virtual void TearDown() {
    data_file.close();
  }

};

TEST_F(HllDruidTest, TestEmpty)
{

  unsigned char bufferDruid[1031] = {0};
  HllDruid hll = HllDruid::wrapRawBuffer(bufferDruid, sizeof(bufferDruid) / sizeof(char));
  hll.reset();

  EXPECT_EQ(hll.approximateCountDistinct(), 0);

  unsigned char synop[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  hll.fold(synop, sizeof(synop) / sizeof(char));

  EXPECT_EQ(hll.approximateCountDistinct(), 0);
}

/**
 * Test approximate count is working properly
 *
 **/
TEST_F(HllDruidTest, TestApproximateCounts) {
  const std::vector<size_t> testCardinalities = {100, 200, 1000, 100000, 500000};
  const double expectedMaxError = 0.05; // we expect error to stay within 5% of real cardinality
  std::set<uint64_t> ids;
  for(uint64_t id; data_file >> id;) {
    ids.insert(id);
  }

  size_t availableIds = ids.size();
  EXPECT_GT(availableIds, 500000); // we have enough unique IDs in the file ()

  unsigned char bufferDruid[1031] = {0};
  HllDruid hll = HllDruid::wrapRawBuffer(bufferDruid, sizeof(bufferDruid) / sizeof(char));
  hll.reset();

  for (size_t nCard = 0; nCard < testCardinalities.size(); ++nCard) {
    size_t itemsAdded = 0;
    uint64_t expectedMinimum = static_cast<uint64_t>( (1.0 - expectedMaxError) * testCardinalities[nCard]);
    uint64_t expectedMaximum = static_cast<uint64_t>( (1.0 + expectedMaxError) * testCardinalities[nCard]);
    for(uint64_t id: ids) {
      if (itemsAdded++ < testCardinalities[nCard]) {
        hll.add<uint64_t>(id);
      } else {
        break;
      }
    }

    EXPECT_GE(hll.approximateCountDistinct(), expectedMinimum);
    EXPECT_LE(hll.approximateCountDistinct(), expectedMaximum);
  }
}

TEST_F(HllDruidTest, TestSerializeSparse)
{
  unsigned char bufferDruid[1031] = {0};
  HllDruid hll = HllDruid::wrapRawBuffer(bufferDruid, sizeof(bufferDruid) / sizeof(char));
  hll.reset();

  EXPECT_EQ(hll.approximateCountDistinct(), 0);

  unsigned char synop[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  hll.fold(synop, sizeof(synop) / sizeof(char));
  for (int i = 240; i < 440; i+=2) {
    hll.add<uint64_t>(i);
    unsigned char sparseBufferDruid[1031] = {0};
    size_t sparseLength = 0;
    hll.serialize(sparseBufferDruid, sparseLength);

    unsigned char otherBufferDruid[1031] = {0};
    HllDruid hll2 = HllDruid::wrapRawBuffer(otherBufferDruid, sizeof(otherBufferDruid) / sizeof(char));
    hll2.reset();
    hll2.fold(sparseBufferDruid, sparseLength);

    EXPECT_EQ(hll.approximateCountDistinct(), hll2.approximateCountDistinct());
  }
}

TEST_F(HllDruidTest, TestDeserializeSparseRegisterIdBoundaries)
{
  uint8_t buffer[1031] = {0};
  HllDruid hll = HllDruid::wrapRawBuffer(buffer, sizeof(buffer));
  hll.reset();

  struct {
    Header header;
    uint16_t registerId;
    uint8_t registerValue;
  } __packed__ other = {
    .header = { .version = 1 },
    .registerId = 0,
    .registerValue = 1, // must not be 0!
  };

  // Last allowed index is length-1, so fold should succeed
  other.registerId = bswap_16(NUM_BYTES_FOR_BUCKETS + NUM_HEADER_BYTES - 1);
  EXPECT_NO_THROW(hll.fold((uint8_t*)&other, sizeof(other)));

  // Last allowed index is length-1, so fold should raise an exception
  other.registerId = bswap_16(NUM_BYTES_FOR_BUCKETS + NUM_HEADER_BYTES);
  EXPECT_THROW(hll.fold((uint8_t*)&other, sizeof(other)), std::runtime_error);
}

} // namespace
