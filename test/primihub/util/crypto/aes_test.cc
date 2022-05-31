#include "gtest/gtest.h"
#include "src/primihub/util/crypto/aes/aes.h"

using namespace primihub;

TEST(aes_test, ecbEncCounterMode) {
  std::vector<block> mBuffer;

  uint64_t bufferSize = 512;
  mBuffer.resize(bufferSize);
  uint64_t mBlockIdx = 0x0;
  AES_Type mAes;
  mAes.ecbEncCounterMode(mBlockIdx, mBuffer.size(), mBuffer.data());

  std::cout << "mBuffer: " << mBuffer[0] << std::endl;
}