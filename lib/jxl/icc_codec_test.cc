// Copyright (c) the JPEG XL Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "lib/jxl/icc_codec.h"

#include <string>

#include "gtest/gtest.h"
#include "lib/jxl/base/span.h"

namespace jxl {
namespace {

void TestProfile(const PaddedBytes& icc) {
  BitWriter writer;
  ASSERT_TRUE(WriteICC(icc, &writer, 0, nullptr));
  writer.ZeroPadToByte();
  PaddedBytes dec;
  BitReader reader(writer.GetSpan());
  ASSERT_TRUE(ReadICC(&reader, &dec));
  ASSERT_TRUE(reader.Close());
  EXPECT_EQ(icc.size(), dec.size());
  if (icc.size() == dec.size()) {
    for (size_t i = 0; i < icc.size(); i++) {
      EXPECT_EQ(icc[i], dec[i]);
      if (icc[i] != dec[i]) break;  // One output is enough
    }
  }
}

void TestProfile(const std::string& icc) {
  PaddedBytes bytes(icc.size());
  for (size_t i = 0; i < icc.size(); i++) {
    bytes[i] = icc[i];
  }
  TestProfile(bytes);
}

// Valid profile from one of the images output by the decoder.
static const unsigned char kTestProfile[] = {
    0x00, 0x00, 0x03, 0x80, 0x6c, 0x63, 0x6d, 0x73, 0x04, 0x30, 0x00, 0x00,
    0x6d, 0x6e, 0x74, 0x72, 0x52, 0x47, 0x42, 0x20, 0x58, 0x59, 0x5a, 0x20,
    0x07, 0xe3, 0x00, 0x04, 0x00, 0x1d, 0x00, 0x0f, 0x00, 0x32, 0x00, 0x2e,
    0x61, 0x63, 0x73, 0x70, 0x41, 0x50, 0x50, 0x4c, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0xf6, 0xd6,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xd3, 0x2d, 0x6c, 0x63, 0x6d, 0x73,
    0x5f, 0x07, 0x0d, 0x3e, 0x4d, 0x32, 0xf2, 0x6e, 0x5d, 0x77, 0x26, 0xcc,
    0x23, 0xb0, 0x6a, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d,
    0x64, 0x65, 0x73, 0x63, 0x00, 0x00, 0x01, 0x20, 0x00, 0x00, 0x00, 0x42,
    0x63, 0x70, 0x72, 0x74, 0x00, 0x00, 0x01, 0x64, 0x00, 0x00, 0x01, 0x00,
    0x77, 0x74, 0x70, 0x74, 0x00, 0x00, 0x02, 0x64, 0x00, 0x00, 0x00, 0x14,
    0x63, 0x68, 0x61, 0x64, 0x00, 0x00, 0x02, 0x78, 0x00, 0x00, 0x00, 0x2c,
    0x72, 0x58, 0x59, 0x5a, 0x00, 0x00, 0x02, 0xa4, 0x00, 0x00, 0x00, 0x14,
    0x62, 0x58, 0x59, 0x5a, 0x00, 0x00, 0x02, 0xb8, 0x00, 0x00, 0x00, 0x14,
    0x67, 0x58, 0x59, 0x5a, 0x00, 0x00, 0x02, 0xcc, 0x00, 0x00, 0x00, 0x14,
    0x72, 0x54, 0x52, 0x43, 0x00, 0x00, 0x02, 0xe0, 0x00, 0x00, 0x00, 0x20,
    0x67, 0x54, 0x52, 0x43, 0x00, 0x00, 0x02, 0xe0, 0x00, 0x00, 0x00, 0x20,
    0x62, 0x54, 0x52, 0x43, 0x00, 0x00, 0x02, 0xe0, 0x00, 0x00, 0x00, 0x20,
    0x63, 0x68, 0x72, 0x6d, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x24,
    0x64, 0x6d, 0x6e, 0x64, 0x00, 0x00, 0x03, 0x24, 0x00, 0x00, 0x00, 0x28,
    0x64, 0x6d, 0x64, 0x64, 0x00, 0x00, 0x03, 0x4c, 0x00, 0x00, 0x00, 0x32,
    0x6d, 0x6c, 0x75, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x0c, 0x65, 0x6e, 0x55, 0x53, 0x00, 0x00, 0x00, 0x26,
    0x00, 0x00, 0x00, 0x1c, 0x00, 0x52, 0x00, 0x47, 0x00, 0x42, 0x00, 0x5f,
    0x00, 0x44, 0x00, 0x36, 0x00, 0x35, 0x00, 0x5f, 0x00, 0x53, 0x00, 0x52,
    0x00, 0x47, 0x00, 0x5f, 0x00, 0x52, 0x00, 0x65, 0x00, 0x6c, 0x00, 0x5f,
    0x00, 0x37, 0x00, 0x30, 0x00, 0x39, 0x00, 0x00, 0x6d, 0x6c, 0x75, 0x63,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0c,
    0x65, 0x6e, 0x55, 0x53, 0x00, 0x00, 0x00, 0xe4, 0x00, 0x00, 0x00, 0x1c,
    0x00, 0x43, 0x00, 0x6f, 0x00, 0x70, 0x00, 0x79, 0x00, 0x72, 0x00, 0x69,
    0x00, 0x67, 0x00, 0x68, 0x00, 0x74, 0x00, 0x20, 0x00, 0x32, 0x00, 0x30,
    0x00, 0x31, 0x00, 0x38, 0x00, 0x20, 0x00, 0x47, 0x00, 0x6f, 0x00, 0x6f,
    0x00, 0x67, 0x00, 0x6c, 0x00, 0x65, 0x00, 0x20, 0x00, 0x4c, 0x00, 0x4c,
    0x00, 0x43, 0x00, 0x2c, 0x00, 0x20, 0x00, 0x43, 0x00, 0x43, 0x00, 0x2d,
    0x00, 0x42, 0x00, 0x59, 0x00, 0x2d, 0x00, 0x53, 0x00, 0x41, 0x00, 0x20,
    0x00, 0x33, 0x00, 0x2e, 0x00, 0x30, 0x00, 0x20, 0x00, 0x55, 0x00, 0x6e,
    0x00, 0x70, 0x00, 0x6f, 0x00, 0x72, 0x00, 0x74, 0x00, 0x65, 0x00, 0x64,
    0x00, 0x20, 0x00, 0x6c, 0x00, 0x69, 0x00, 0x63, 0x00, 0x65, 0x00, 0x6e,
    0x00, 0x73, 0x00, 0x65, 0x00, 0x28, 0x00, 0x68, 0x00, 0x74, 0x00, 0x74,
    0x00, 0x70, 0x00, 0x73, 0x00, 0x3a, 0x00, 0x2f, 0x00, 0x2f, 0x00, 0x63,
    0x00, 0x72, 0x00, 0x65, 0x00, 0x61, 0x00, 0x74, 0x00, 0x69, 0x00, 0x76,
    0x00, 0x65, 0x00, 0x63, 0x00, 0x6f, 0x00, 0x6d, 0x00, 0x6d, 0x00, 0x6f,
    0x00, 0x6e, 0x00, 0x73, 0x00, 0x2e, 0x00, 0x6f, 0x00, 0x72, 0x00, 0x67,
    0x00, 0x2f, 0x00, 0x6c, 0x00, 0x69, 0x00, 0x63, 0x00, 0x65, 0x00, 0x6e,
    0x00, 0x73, 0x00, 0x65, 0x00, 0x73, 0x00, 0x2f, 0x00, 0x62, 0x00, 0x79,
    0x00, 0x2d, 0x00, 0x73, 0x00, 0x61, 0x00, 0x2f, 0x00, 0x33, 0x00, 0x2e,
    0x00, 0x30, 0x00, 0x2f, 0x00, 0x6c, 0x00, 0x65, 0x00, 0x67, 0x00, 0x61,
    0x00, 0x6c, 0x00, 0x63, 0x00, 0x6f, 0x00, 0x64, 0x00, 0x65, 0x00, 0x29,
    0x58, 0x59, 0x5a, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf6, 0xd6,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xd3, 0x2d, 0x73, 0x66, 0x33, 0x32,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x42, 0x00, 0x00, 0x05, 0xde,
    0xff, 0xff, 0xf3, 0x25, 0x00, 0x00, 0x07, 0x93, 0x00, 0x00, 0xfd, 0x90,
    0xff, 0xff, 0xfb, 0xa1, 0xff, 0xff, 0xfd, 0xa2, 0x00, 0x00, 0x03, 0xdc,
    0x00, 0x00, 0xc0, 0x6e, 0x58, 0x59, 0x5a, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x6f, 0xa0, 0x00, 0x00, 0x38, 0xf5, 0x00, 0x00, 0x03, 0x90,
    0x58, 0x59, 0x5a, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x9f,
    0x00, 0x00, 0x0f, 0x84, 0x00, 0x00, 0xb6, 0xc4, 0x58, 0x59, 0x5a, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x97, 0x00, 0x00, 0xb7, 0x87,
    0x00, 0x00, 0x18, 0xd9, 0x70, 0x61, 0x72, 0x61, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x38, 0xe4, 0x00, 0x00, 0xe8, 0xf0,
    0x00, 0x00, 0x17, 0x10, 0x00, 0x00, 0x38, 0xe4, 0x00, 0x00, 0x14, 0xbc,
    0x63, 0x68, 0x72, 0x6d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
    0x00, 0x00, 0xa3, 0xd7, 0x00, 0x00, 0x54, 0x7c, 0x00, 0x00, 0x4c, 0xcd,
    0x00, 0x00, 0x99, 0x9a, 0x00, 0x00, 0x26, 0x67, 0x00, 0x00, 0x0f, 0x5c,
    0x6d, 0x6c, 0x75, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x0c, 0x65, 0x6e, 0x55, 0x53, 0x00, 0x00, 0x00, 0x0c,
    0x00, 0x00, 0x00, 0x1c, 0x00, 0x47, 0x00, 0x6f, 0x00, 0x6f, 0x00, 0x67,
    0x00, 0x6c, 0x00, 0x65, 0x6d, 0x6c, 0x75, 0x63, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0c, 0x65, 0x6e, 0x55, 0x53,
    0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x49, 0x00, 0x6d,
    0x00, 0x61, 0x00, 0x67, 0x00, 0x65, 0x00, 0x20, 0x00, 0x63, 0x00, 0x6f,
    0x00, 0x64, 0x00, 0x65, 0x00, 0x63, 0x00, 0x00,
};

}  // namespace

TEST(IccCodecTest, Icc) {
  // Empty string cannot be tested, encoder checks against writing it.
  TestProfile("a");
  TestProfile("ab");
  TestProfile("aaaa");

  {
    // Exactly the ICC header size
    PaddedBytes profile(128);
    for (size_t i = 0; i < 128; i++) {
      profile[i] = 0;
    }
    TestProfile(profile);
  }

  {
    PaddedBytes profile;
    profile.append(kTestProfile, kTestProfile + sizeof(kTestProfile));
    TestProfile(profile);
  }

  // Test substrings of full profile
  {
    PaddedBytes profile;
    for (size_t i = 0; i <= 256; i++) {
      profile.push_back(kTestProfile[i]);
      TestProfile(profile);
    }
  }
}

}  // namespace jxl