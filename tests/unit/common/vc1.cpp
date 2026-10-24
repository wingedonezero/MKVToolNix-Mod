#include "common/common_pch.h"

#include "common/vc1.h"

#include "tests/unit/init.h"

namespace {

// A frame: the frame start code, then the header bits given as a string of
// '0' / '1' (padded with zero bits to whole bytes).
std::vector<uint8_t>
frame_with_header(std::string const &bits) {
  std::vector<uint8_t> buffer{ 0x00, 0x00, 0x01, 0x0d };
  uint8_t byte{};
  auto num_bits = 0u;

  for (auto bit : bits) {
    byte = (byte << 1) | (bit == '1' ? 1 : 0);
    if (++num_bits == 8) {
      buffer.push_back(byte);
      byte     = 0;
      num_bits = 0;
    }
  }
  if (num_bits)
    buffer.push_back(byte << (8 - num_bits));

  // the rest of a frame
  buffer.insert(buffer.end(), 4, 0x00);

  return buffer;
}

mtx::vc1::frame_header_t
parse(std::string const &bits,
      mtx::vc1::sequence_header_t const &seqhdr) {
  auto frame = frame_with_header(bits);
  mtx::vc1::frame_header_t frame_header;
  auto copy  = seqhdr;

  EXPECT_TRUE(mtx::vc1::parse_frame_header(frame.data(), frame.size(), frame_header, copy));

  return frame_header;
}

mtx::vc1::sequence_header_t
interlaced_pulldown_sequence(bool tf_counter = false) {
  mtx::vc1::sequence_header_t seqhdr;
  seqhdr.pulldown_flag   = true;
  seqhdr.interlace_flag  = true;
  seqhdr.tf_counter_flag = tf_counter;

  return seqhdr;
}

// SMPTE 421M: a skipped picture's header carries TFCNTR and the pulldown
// fields (TFF / RFF, or RPTFRM) like any other picture's.

TEST(VC1, SkippedFrameRepeatsFirstField) {
  // FCM 0 (progressive coding), PTYPE 1111 (skipped), TFF 1, RFF 1
  auto fh = parse("0" "1111" "1" "1", interlaced_pulldown_sequence());

  EXPECT_EQ(mtx::vc1::FRAME_TYPE_P_SKIPPED, fh.frame_type);
  EXPECT_TRUE(fh.top_field_first_flag);
  EXPECT_TRUE(fh.repeat_first_field_flag);
}

TEST(VC1, SkippedFrameWithoutRepeatedField) {
  // FCM 10 (interlaced frame), PTYPE 1111, TFF 0, RFF 0
  auto fh = parse("10" "1111" "0" "0", interlaced_pulldown_sequence());

  EXPECT_EQ(mtx::vc1::FRAME_TYPE_P_SKIPPED, fh.frame_type);
  EXPECT_FALSE(fh.top_field_first_flag);
  EXPECT_FALSE(fh.repeat_first_field_flag);
}

TEST(VC1, SkippedFrameWithFrameCounter) {
  // FCM 0, PTYPE 1111, TFCNTR 0xa5, TFF 0, RFF 1
  auto fh = parse("0" "1111" "10100101" "0" "1", interlaced_pulldown_sequence(true));

  EXPECT_EQ(mtx::vc1::FRAME_TYPE_P_SKIPPED, fh.frame_type);
  EXPECT_EQ(0xa5, fh.tf_counter);
  EXPECT_FALSE(fh.top_field_first_flag);
  EXPECT_TRUE(fh.repeat_first_field_flag);
}

TEST(VC1, SkippedProgressiveFrameRepeatsFrames) {
  // progressive sequence with pulldown: PTYPE 1111, RPTFRM 2
  mtx::vc1::sequence_header_t seqhdr;
  seqhdr.pulldown_flag = true;
  auto fh = parse("1111" "10", seqhdr);

  EXPECT_EQ(mtx::vc1::FRAME_TYPE_P_SKIPPED, fh.frame_type);
  EXPECT_EQ(2, fh.repeat_frame);
}

TEST(VC1, StartsWithStartCode) {
  auto check = [](std::vector<uint8_t> const &bytes) {
    return mtx::vc1::starts_with_start_code(bytes.data(), bytes.size());
  };

  EXPECT_TRUE(check({ 0x00, 0x00, 0x01, 0x0d, 0x12 }));
  EXPECT_TRUE(check({ 0x00, 0x00, 0x00, 0x01, 0x0f }));
  EXPECT_FALSE(check({ 0x00, 0x01, 0x0d }));
  EXPECT_FALSE(check({ 0x40, 0x00, 0x00, 0x01 }));
  EXPECT_FALSE(check({ 0x00, 0x00, 0x00 }));
  EXPECT_FALSE(check({}));
}

// a BITMAPINFOHEADER (40 bytes) followed by the given extra data
memory_cptr
vfw_codec_private(std::vector<uint8_t> const &extra) {
  std::vector<uint8_t> bytes(40, 0x00);
  bytes.insert(bytes.end(), extra.begin(), extra.end());

  return memory_c::clone(bytes.data(), bytes.size());
}

TEST(VC1, HeadersFromVfwCodecPrivate) {
  std::vector<uint8_t> headers{ 0x00, 0x00, 0x01, 0x0f, 0xca, 0xfe, 0x00, 0x00, 0x01, 0x0e, 0xbe, 0xef };

  // mkvmerge: a zero byte, then the headers
  auto with_zero = headers;
  with_zero.insert(with_zero.begin(), 0x00);
  auto from_mkvmerge = mtx::vc1::headers_from_vfw_codec_private(*vfw_codec_private(with_zero));
  ASSERT_TRUE(!!from_mkvmerge);
  EXPECT_EQ(*memory_c::clone(headers.data(), headers.size()), *from_mkvmerge);

  // FFmpeg: the headers right after the BITMAPINFOHEADER
  auto from_ffmpeg = mtx::vc1::headers_from_vfw_codec_private(*vfw_codec_private(headers));
  ASSERT_TRUE(!!from_ffmpeg);
  EXPECT_EQ(*memory_c::clone(headers.data(), headers.size()), *from_ffmpeg);

  // none
  EXPECT_FALSE(!!mtx::vc1::headers_from_vfw_codec_private(*vfw_codec_private({})));
  EXPECT_FALSE(!!mtx::vc1::headers_from_vfw_codec_private(*vfw_codec_private({ 0x12, 0x34, 0x56, 0x78 })));
}

TEST(VC1, CodedFramesUnchanged) {
  // FCM 0, PTYPE 0 (P), TFF 1, RFF 1
  auto p = parse("0" "0" "1" "1", interlaced_pulldown_sequence());
  EXPECT_EQ(mtx::vc1::FRAME_TYPE_P, p.frame_type);
  EXPECT_TRUE(p.top_field_first_flag);
  EXPECT_TRUE(p.repeat_first_field_flag);

  // FCM 0, PTYPE 10 (B), TFF 0, RFF 0
  auto b = parse("0" "10" "0" "0", interlaced_pulldown_sequence());
  EXPECT_EQ(mtx::vc1::FRAME_TYPE_B, b.frame_type);
  EXPECT_FALSE(b.top_field_first_flag);
  EXPECT_FALSE(b.repeat_first_field_flag);
}

}
