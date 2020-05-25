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

#ifndef JXL_QUANT_WEIGHTS_H_
#define JXL_QUANT_WEIGHTS_H_

#include <stdint.h>
#include <string.h>

#include <array>
#include <hwy/aligned_allocator.h>
#include <utility>
#include <vector>

#include "jxl/ac_strategy.h"
#include "jxl/aux_out_fwd.h"
#include "jxl/base/cache_aligned.h"
#include "jxl/base/compiler_specific.h"
#include "jxl/base/span.h"
#include "jxl/base/status.h"
#include "jxl/common.h"
#include "jxl/dec_bit_reader.h"
#include "jxl/enc_bit_writer.h"
#include "jxl/enc_params.h"
#include "jxl/image.h"

namespace jxl {

template <typename T, size_t N>
constexpr T ArraySum(T (&a)[N], size_t i = N - 1) {
  static_assert(N > 0, "Trying to compute the sum of an empty array");
  return i == 0 ? a[0] : a[i] + ArraySum(a, i - 1);
}

static constexpr size_t kMaxQuantTableSize = kDCTBlockSize * 16;
static constexpr size_t kNumPredefinedTables = 1;
static constexpr size_t kCeilLog2NumPredefinedTables = 0;
static constexpr size_t kLog2NumQuantModes = 3;

struct DctQuantWeightParams {
  static constexpr size_t kLog2MaxDistanceBands = 4;
  static constexpr size_t kMaxDistanceBands = 1 + (1 << kLog2MaxDistanceBands);
  typedef std::array<std::array<float, kMaxDistanceBands>, 3>
      DistanceBandsArray;

  size_t num_distance_bands = 0;
  DistanceBandsArray distance_bands = {};

  constexpr DctQuantWeightParams() : num_distance_bands(0) {}

  constexpr DctQuantWeightParams(const DistanceBandsArray& dist_bands,
                                 size_t num_dist_bands)
      : num_distance_bands(num_dist_bands), distance_bands(dist_bands) {}

  template <size_t num_dist_bands>
  explicit DctQuantWeightParams(const float dist_bands[3][num_dist_bands]) {
    num_distance_bands = num_dist_bands;
    for (size_t c = 0; c < 3; c++) {
      memcpy(distance_bands[c].data(), dist_bands[c],
             sizeof(float) * num_dist_bands);
    }
  }
};

// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
struct QuantEncodingInternal {
  enum Mode {
    kQuantModeLibrary,
    kQuantModeID,
    kQuantModeDCT2,
    kQuantModeDCT4,
    kQuantModeDCT4X8,
    kQuantModeAFV,
    kQuantModeDCT,
    kQuantModeRAW,
  };

  template <Mode mode>
  struct Tag {};

  typedef std::array<std::array<float, 3>, 3> IdWeights;
  typedef std::array<std::array<float, 6>, 3> DCT2Weights;
  typedef std::array<std::array<float, 2>, 3> DCT4Multipliers;
  typedef std::array<std::array<float, 9>, 3> AFVWeights;
  typedef std::array<float, 3> DCT4x8Multipliers;

  static constexpr QuantEncodingInternal Library(uint8_t predefined) {
    return ((predefined < kNumPredefinedTables) ||
            ::jxl::Abort(__FILE__, __LINE__,
                         "Assert predefined < kNumPredefinedTables")),
           QuantEncodingInternal(Tag<kQuantModeLibrary>(), predefined);
  }
  constexpr QuantEncodingInternal(Tag<kQuantModeLibrary> /* tag */,
                                  uint8_t predefined)
      : mode(kQuantModeLibrary), predefined(predefined) {}

  // Identity
  // xybweights is an array of {xweights, yweights, bweights}.
  static constexpr QuantEncodingInternal Identity(const IdWeights& xybweights) {
    return QuantEncodingInternal(Tag<kQuantModeID>(), xybweights);
  }
  constexpr QuantEncodingInternal(Tag<kQuantModeID> /* tag */,
                                  const IdWeights& xybweights)
      : mode(kQuantModeID), idweights(xybweights) {}

  // DCT2
  static constexpr QuantEncodingInternal DCT2(const DCT2Weights& xybweights) {
    return QuantEncodingInternal(Tag<kQuantModeDCT2>(), xybweights);
  }
  constexpr QuantEncodingInternal(Tag<kQuantModeDCT2> /* tag */,
                                  const DCT2Weights& xybweights)
      : mode(kQuantModeDCT2), dct2weights(xybweights) {}

  // DCT4
  static constexpr QuantEncodingInternal DCT4(
      const DctQuantWeightParams& params, const DCT4Multipliers& xybmul) {
    return QuantEncodingInternal(Tag<kQuantModeDCT4>(), params, xybmul);
  }
  constexpr QuantEncodingInternal(Tag<kQuantModeDCT4> /* tag */,
                                  const DctQuantWeightParams& params,
                                  const DCT4Multipliers& xybmul)
      : mode(kQuantModeDCT4), dct_params(params), dct4multipliers(xybmul) {}

  // DCT4x8
  static constexpr QuantEncodingInternal DCT4X8(
      const DctQuantWeightParams& params, const DCT4x8Multipliers& xybmul) {
    return QuantEncodingInternal(Tag<kQuantModeDCT4X8>(), params, xybmul);
  }
  constexpr QuantEncodingInternal(Tag<kQuantModeDCT4X8> /* tag */,
                                  const DctQuantWeightParams& params,
                                  const DCT4x8Multipliers& xybmul)
      : mode(kQuantModeDCT4X8), dct_params(params), dct4x8multipliers(xybmul) {}

  // DCT
  static constexpr QuantEncodingInternal DCT(
      const DctQuantWeightParams& params) {
    return QuantEncodingInternal(Tag<kQuantModeDCT>(), params);
  }
  constexpr QuantEncodingInternal(Tag<kQuantModeDCT> /* tag */,
                                  const DctQuantWeightParams& params)
      : mode(kQuantModeDCT), dct_params(params) {}

  // AFV
  static constexpr QuantEncodingInternal AFV(
      const DctQuantWeightParams& params4x8,
      const DctQuantWeightParams& params4x4, const AFVWeights& weights) {
    return QuantEncodingInternal(Tag<kQuantModeAFV>(), params4x8, params4x4,
                                 weights);
  }
  constexpr QuantEncodingInternal(Tag<kQuantModeAFV> /* tag */,
                                  const DctQuantWeightParams& params4x8,
                                  const DctQuantWeightParams& params4x4,
                                  const AFVWeights& weights)
      : mode(kQuantModeAFV),
        dct_params(params4x8),
        afv_weights(weights),
        dct_params_afv_4x4(params4x4) {}

  // This constructor is not constexpr so it can't be used in any of the
  // constexpr cases above.
  explicit QuantEncodingInternal(Mode mode) : mode(mode) {}

  Mode mode;

  // Weights for DCT4+ tables.
  DctQuantWeightParams dct_params;

  union {
    // Weights for identity.
    IdWeights idweights;

    // Weights for DCT2.
    DCT2Weights dct2weights;

    // Extra multipliers for coefficients 01/10 and 11 for DCT4 and AFV.
    DCT4Multipliers dct4multipliers;

    // Weights for AFV. {0, 1} are used directly for coefficients (0, 1) and (1,
    // 0);  {2, 3, 4} are used directly corner DC, (1,0) - (0,1) and (0, 1) +
    // (1, 0) - (0, 0) inside the AFV block. Values from 5 to 8 are interpolated
    // as in GetQuantWeights for DC and are used for other coefficients.
    AFVWeights afv_weights = {};

    // Extra multipliers for coefficients 01 or 10 for DCT4X8 and DCT8X4.
    DCT4x8Multipliers dct4x8multipliers;

    // Only used in kQuantModeRAW mode.
    struct {
      // explicit quantization table (like in JPEG)
      std::vector<int>* qtable = nullptr;
      int qtable_den_shift = 0;
    } qraw;
  };

  // Weights for 4x4 sub-block in AFV.
  DctQuantWeightParams dct_params_afv_4x4;

  union {
    // Which predefined table to use. Only used if mode is kQuantModeLibrary.
    uint8_t predefined = 0;

    // Which other quant table to copy; must copy from a table that comes before
    // the current one. Only used if mode is kQuantModeCopy.
    uint8_t source;
  };
};

class QuantEncoding final : public QuantEncodingInternal {
 public:
  QuantEncoding(const QuantEncoding& other)
      : QuantEncodingInternal(
            static_cast<const QuantEncodingInternal&>(other)) {
    if (mode == kQuantModeRAW && qraw.qtable) {
      // Need to make a copy of the passed *qtable.
      qraw.qtable = new std::vector<int>(*other.qraw.qtable);
    }
  }
  QuantEncoding(QuantEncoding&& other) noexcept
      : QuantEncodingInternal(
            static_cast<const QuantEncodingInternal&>(other)) {
    // Steal the qtable from the other object if any.
    if (mode == kQuantModeRAW) {
      other.qraw.qtable = nullptr;
    }
  }
  QuantEncoding& operator=(const QuantEncoding& other) {
    if (mode == kQuantModeRAW && qraw.qtable) {
      delete qraw.qtable;
    }
    *static_cast<QuantEncodingInternal*>(this) =
        QuantEncodingInternal(static_cast<const QuantEncodingInternal&>(other));
    if (mode == kQuantModeRAW && qraw.qtable) {
      // Need to make a copy of the passed *qtable.
      qraw.qtable = new std::vector<int>(*other.qraw.qtable);
    }
    return *this;
  }

  ~QuantEncoding() {
    if (mode == kQuantModeRAW && qraw.qtable) {
      delete qraw.qtable;
    }
  }

  // Wrappers of the QuantEncodingInternal:: static functions that return a
  // QuantEncoding instead. This is using the explicit and private cast from
  // QuantEncodingInternal to QuantEncoding, which would be inlined anyway.
  // In general, you should use this wrappers. The only reason to directly
  // create a QuantEncodingInternal instance is if you need a constexpr version
  // of this class. Note that RAW() is not supported in that case since it uses
  // a std::vector.
  static QuantEncoding Library(uint8_t predefined) {
    return QuantEncoding(QuantEncodingInternal::Library(predefined));
  }
  static QuantEncoding Identity(const IdWeights& xybweights) {
    return QuantEncoding(QuantEncodingInternal::Identity(xybweights));
  }
  static QuantEncoding DCT2(const DCT2Weights& xybweights) {
    return QuantEncoding(QuantEncodingInternal::DCT2(xybweights));
  }
  static QuantEncoding DCT4(const DctQuantWeightParams& params,
                            const DCT4Multipliers& xybmul) {
    return QuantEncoding(QuantEncodingInternal::DCT4(params, xybmul));
  }
  static QuantEncoding DCT4X8(const DctQuantWeightParams& params,
                              const DCT4x8Multipliers& xybmul) {
    return QuantEncoding(QuantEncodingInternal::DCT4X8(params, xybmul));
  }
  static QuantEncoding DCT(const DctQuantWeightParams& params) {
    return QuantEncoding(QuantEncodingInternal::DCT(params));
  }
  static QuantEncoding AFV(const DctQuantWeightParams& params4x8,
                           const DctQuantWeightParams& params4x4,
                           const AFVWeights& weights) {
    return QuantEncoding(
        QuantEncodingInternal::AFV(params4x8, params4x4, weights));
  }

  // RAW, note that this one is not a constexpr one.
  static QuantEncoding RAW(const std::vector<int>& qtable, int shift = 0) {
    QuantEncoding encoding(kQuantModeRAW);
    encoding.qraw.qtable = new std::vector<int>();
    *encoding.qraw.qtable = qtable;
    encoding.qraw.qtable_den_shift = shift;
    return encoding;
  }

 private:
  explicit QuantEncoding(const QuantEncodingInternal& other)
      : QuantEncodingInternal(other) {}

  explicit QuantEncoding(QuantEncodingInternal::Mode mode)
      : QuantEncodingInternal(mode) {}
};

// A constexpr QuantEncodingInternal instance is often downcasted to the
// QuantEncoding subclass even if the instance wasn't an instance of the
// subclass. This is safe because user will upcast to QuantEncodingInternal to
// access any of its members.
static_assert(sizeof(QuantEncoding) == sizeof(QuantEncodingInternal),
              "Don't add any members to QuantEncoding");

// Let's try to keep these 2**N for possible future simplicity.
const float kInvDCQuant[3] = {
    4096.0f,
    512.0f,
    256.0f,
};

const float kDCQuant[3] = {
    1.0f / kInvDCQuant[0],
    1.0f / kInvDCQuant[1],
    1.0f / kInvDCQuant[2],
};

class DequantMatrices {
 public:
  enum QuantTable : size_t {
    DCT = 0,
    IDENTITY,
    DCT2X2,
    DCT4X4,
    DCT16X16,
    DCT32X32,
    // DCT16X8
    DCT8X16,
    // DCT32X8
    DCT8X32,
    // DCT32X16
    DCT16X32,
    DCT4X8,
    // DCT8X4
    AFV0,
    // AFV1
    // AFV2
    // AFV3
    kNum
  };

  static_assert(AcStrategy::kNumValidStrategies == 18,
                "Update this array when adding or removing AC strategies.");
  static constexpr QuantTable kQuantTable[] = {
      QuantTable::DCT,     QuantTable::IDENTITY, QuantTable::DCT2X2,
      QuantTable::DCT4X4,  QuantTable::DCT16X16, QuantTable::DCT32X32,
      QuantTable::DCT8X16, QuantTable::DCT8X16,  QuantTable::DCT8X32,
      QuantTable::DCT8X32, QuantTable::DCT16X32, QuantTable::DCT16X32,
      QuantTable::DCT4X8,  QuantTable::DCT4X8,   QuantTable::AFV0,
      QuantTable::AFV0,    QuantTable::AFV0,     QuantTable::AFV0,
  };

  DequantMatrices()
      // TODO(janwas): integrate JpegxlMemoryManager pointers
      : table_(hwy::AllocateAligned<float>(2 * kTotalTableSize)) {
    encodings_.resize(size_t(QuantTable::kNum), QuantEncoding::Library(0));
    for (size_t i = 0; i < size_t(QuantTable::kNum); i++) {
      encodings_[i] = QuantEncoding::Library(0);
    }
    // Default quantization tables need to be valid.
    JXL_CHECK(Compute());
  }

  static const QuantEncoding* Library();

  typedef std::array<QuantEncodingInternal, kNumPredefinedTables * kNum>
      DequantLibraryInternal;
  // Return the array of library kNumPredefinedTables QuantEncoding entries as
  // a constexpr array. Use Library() to obtain a pointer to the copy in the
  // .cc file.
  static const DequantLibraryInternal LibraryInit();

  JXL_INLINE size_t MatrixOffset(size_t quant_kind, size_t c) const {
    JXL_DASSERT(quant_kind < AcStrategy::kNumValidStrategies);
    return table_offsets_[quant_kind * 3 + c];
  }

  // Returns aligned memory.
  JXL_INLINE const float* Matrix(size_t quant_kind, size_t c) const {
    JXL_DASSERT(quant_kind < AcStrategy::kNumValidStrategies);
    return &table_[MatrixOffset(quant_kind, c)];
  }

  JXL_INLINE const float* InvMatrix(size_t quant_kind, size_t c) const {
    JXL_DASSERT(quant_kind < AcStrategy::kNumValidStrategies);
    return &InvTable()[MatrixOffset(quant_kind, c)];
  }

  JXL_INLINE float DCQuant(size_t c) const { return dc_quant_[c]; }

  JXL_INLINE float InvDCQuant(size_t c) const { return inv_dc_quant_[c]; }

  size_t Size() const { return size_; }

  void SetCustom(const std::vector<QuantEncoding>& encodings) {
    JXL_ASSERT(encodings.size() == kNum);
    encodings_ = encodings;
    // Roundtrip encode/decode the matrices to ensure same values as decoder.
    BitWriter writer;
    JXL_CHECK(Encode(&writer, 0, nullptr));
    writer.ZeroPadToByte();
    BitReader br(writer.GetSpan());
    // Called only in the encoder: should fail only for programmer errors.
    JXL_CHECK(Decode(&br));
    JXL_CHECK(br.Close());
  }

  // For consistency with QuantEncoding, higher values correspond to more
  // precision.
  void SetCustomDC(const float* dc) {
    for (size_t c = 0; c < 3; c++) {
      dc_quant_[c] = 1.0f / dc[c];
      inv_dc_quant_[c] = dc[c];
    }
    // Roundtrip encode/decode DC to ensure same values as decoder.
    BitWriter writer;
    JXL_CHECK(EncodeDC(&writer, 0, nullptr));
    writer.ZeroPadToByte();
    BitReader br(writer.GetSpan());
    // Called only in the encoder: should fail only for programmer errors.
    JXL_CHECK(DecodeDC(&br));
    JXL_CHECK(br.Close());
  }

  Status Encode(BitWriter* writer, size_t layer, AuxOut* aux_out) const;
  Status EncodeDC(BitWriter* writer, size_t layer, AuxOut* aux_out) const;

  Status Decode(BitReader* br);
  Status DecodeDC(BitReader* br);

  const std::vector<QuantEncoding>& encodings() const { return encodings_; }

 private:
  float* InvTable() const { return table_.get() + kTotalTableSize; }

  Status Compute();
  static_assert(kNum == 11,
                "Update this array when adding or removing quant tables.");
  static constexpr size_t required_size_x_[kNum] = {1, 1, 1, 1, 2, 4,
                                                    1, 1, 2, 1, 1};
  static constexpr size_t required_size_y_[kNum] = {1, 1, 1, 1, 2, 4,
                                                    2, 4, 4, 1, 1};
  static constexpr size_t required_size_[kNum] = {1, 1, 1, 1, 4, 16,
                                                  2, 4, 8, 1, 1};
  static constexpr size_t kTotalTableSize =
      ArraySum(required_size_) * kDCTBlockSize * 3;

  // kTotalTableSize entries followed by kTotalTableSize for inv_table
  hwy::AlignedUniquePtr<float[]> table_;
  float dc_quant_[3] = {kDCQuant[0], kDCQuant[1], kDCQuant[2]};
  float inv_dc_quant_[3] = {kInvDCQuant[0], kInvDCQuant[1], kInvDCQuant[2]};
  size_t table_offsets_[AcStrategy::kNumValidStrategies * 3];
  size_t size_;
  std::vector<QuantEncoding> encodings_;
};

void FindBestDequantMatrices(const CompressParams& cparams,
                             const Image3F& opsin,
                             DequantMatrices* dequant_matrices);

}  // namespace jxl

#endif  // JXL_QUANT_WEIGHTS_H_
