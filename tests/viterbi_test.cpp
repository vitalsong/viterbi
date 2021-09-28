// Unit test for ViterbiCodec.
//
// Author: Min Xu <xukmin@gmail.com>
// Date: 01/30/2015

#include "viterbi.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

static std::string randomMessage(int num_bits) {
  std::string msg;
  for (int j = 0; j < num_bits; j++) {
    msg += (std::rand() & 1) + '0';
  }
  return msg;
}

TEST(Viterbi, Poly_7x5_err) {
  ViterbiCodec codec(3, {7, 5});
  ASSERT_EQ(codec.Decode("001110000110011111100010110011"), "010111001010001");

  // Inject 1 error bit.
  ASSERT_EQ(codec.Decode("001110000110011111000010110011"), "010111001010001");
}

TEST(Viterbi, Poly_7x6_err) {
  ViterbiCodec codec(3, {7, 6});
  ASSERT_EQ(codec.Decode("101101010011"), "101100");
}

TEST(Viterbi, Poly_6x5_err) {
  ViterbiCodec codec(3, {6, 5});
  ASSERT_EQ(codec.Decode("01101101110110"), "1001101");

  // Inject 2 error bits.
  ASSERT_EQ(codec.Decode("11101101110010"), "1001101");
}

TEST(Viterbi, Poly_91x117x121_err) {
  ViterbiCodec codec(7, {91, 117, 121});
  ASSERT_EQ(codec.Decode("111100101110001011110101"), "10110111");

  // Inject 4 error bits.
  ASSERT_EQ(codec.Decode("100100101110001011110101"), "10110111");
}

TEST(Viterbi, Voyager_err) {
  ViterbiCodec codec(7, {109, 79});
  auto message = randomMessage(32);
  auto encoded = codec.Encode(message);

  // add 5% errors
  int nerr = encoded.size() * 0.05;
  for (size_t i = 0; i < nerr; i++) {
    int idx = rand() % encoded.size();
    encoded[idx] = (encoded[idx] == '0') ? ('1') : ('0');
  }

  auto decoded = codec.Decode(encoded);
  ASSERT_EQ(message, decoded);
}

// Test the given ViterbiCodec by randomly generating 10 input sequences of
// length 8, 16, 32 respectively, encode and decode them, then test if the
// decoded string is the same as the original input.
void TestViterbiCodecAutomatic(const ViterbiCodec &codec) {
  for (int num_bits = 8; num_bits <= 32; num_bits <<= 1) {
    for (int i = 0; i < 10; i++) {
      std::string message = randomMessage(num_bits);
      std::string encoded = codec.Encode(message);
      std::string decoded = codec.Decode(encoded);
      ASSERT_EQ(decoded, message);
    }
  }
}

TEST(Viterbi, Poly_7x5) {
  ViterbiCodec codec(3, {7, 5});
  TestViterbiCodecAutomatic(codec);
}

TEST(Viterbi, Poly_6x5) {
  ViterbiCodec codec(3, {6, 5});
  TestViterbiCodecAutomatic(codec);
}

TEST(Viterbi, Voyager) {
  // Voyager
  ViterbiCodec codec(7, {109, 79});
  TestViterbiCodecAutomatic(codec);
}

TEST(Viterbi, LTE) {
  // LTE
  ViterbiCodec codec(7, {91, 117, 121});
  TestViterbiCodecAutomatic(codec);
}

TEST(Viterbi, CDMA_2000) {
  // CDMA 2000
  ViterbiCodec codec(9, {501, 441, 331, 315});
  TestViterbiCodecAutomatic(codec);
}

TEST(Viterbi, Cassini) {
  // Cassini / Mars Pathfinder
  ViterbiCodec codec(15, {15, 17817, 20133, 23879, 30451, 32439, 26975});
  TestViterbiCodecAutomatic(codec);
}

int main(int argc, char **argv) {
  std::srand(std::time(NULL));
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
