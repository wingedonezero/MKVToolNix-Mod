#include "common/common_pch.h"

#include <fstream>

#include "common/avc/es_parser.h"
#include "common/endian.h"
#include "common/hevc/es_parser.h"

#include "tests/unit/init.h"

namespace {

// Two small streams (25 frames, a key frame every 6, B frames), each with an
// access unit delimiter in front of every access unit and the parameter sets
// only in front of the first one.
std::string const s_h264 = "tests/unit/data/video/aud-headers-once.h264";
std::string const s_hevc = "tests/unit/data/video/aud-headers-once.hevc";

std::vector<uint8_t>
read_file(std::string const &name) {
  std::ifstream in{name, std::ios::binary};
  return { std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{} };
}

// the NALU types of a frame (NALUs with a 4-byte size in front)
std::vector<int>
nalu_types(mtx::xyzvc::frame_t const &frame,
           bool hevc) {
  std::vector<int> types;
  auto buf  = frame.m_data->get_buffer();
  auto size = frame.m_data->get_size();

  for (std::size_t pos = 0; (pos + 5) <= size; ) {
    auto nalu_size = get_uint32_be(&buf[pos]);
    types.push_back(hevc ? (buf[pos + 4] >> 1) & 0x3f : buf[pos + 4] & 0x1f);
    pos += 4 + nalu_size;
  }

  return types;
}

std::vector<std::vector<int>>
parse(mtx::xyzvc::es_parser_c &parser,
      std::string const &file_name,
      bool hevc) {
  auto data = read_file(file_name);
  EXPECT_FALSE(data.empty());

  parser.force_default_duration(40'000'000);
  parser.add_bytes(data.data(), data.size());
  parser.flush();

  std::vector<std::vector<int>> frames;
  while (parser.frame_available())
    frames.push_back(nalu_types(parser.get_frame(), hevc));

  return frames;
}

TEST(AccessUnitDelimiters, AVCKeptFirstInEveryFrame) {
  mtx::avc::es_parser_c parser;
  auto frames = parse(parser, s_h264, false);

  ASSERT_EQ(25u, frames.size());
  for (auto const &types : frames) {
    ASSERT_FALSE(types.empty());
    EXPECT_EQ(mtx::avc::NALU_TYPE_ACCESS_UNIT, types.front());
    EXPECT_EQ(1, std::count(types.begin(), types.end(), mtx::avc::NALU_TYPE_ACCESS_UNIT));
  }

  // the first frame as in the stream: AUD SPS PPS SEI IDR
  EXPECT_EQ((std::vector<int>{ mtx::avc::NALU_TYPE_ACCESS_UNIT, mtx::avc::NALU_TYPE_SEQ_PARAM, mtx::avc::NALU_TYPE_PIC_PARAM, mtx::avc::NALU_TYPE_SEI, mtx::avc::NALU_TYPE_IDR_SLICE }), frames.front());
}

TEST(AccessUnitDelimiters, HEVCFirstAlsoWhenParameterSetsArePutInFrontOfKeyFrames) {
  mtx::hevc::es_parser_c parser;
  parser.set_normalize_parameter_sets(true);
  auto frames = parse(parser, s_hevc, true);

  ASSERT_EQ(25u, frames.size());

  auto key_frames_with_inserted_parameter_sets = 0;
  for (auto const &types : frames) {
    ASSERT_FALSE(types.empty());
    EXPECT_EQ(mtx::hevc::NALU_TYPE_ACCESS_UNIT, types.front());
    EXPECT_EQ(1, std::count(types.begin(), types.end(), mtx::hevc::NALU_TYPE_ACCESS_UNIT));
    if ((types.size() > 1) && (types[1] == mtx::hevc::NALU_TYPE_VIDEO_PARAM))
      ++key_frames_with_inserted_parameter_sets;
  }

  // the stream has VPS / SPS / PPS before its first key frame only: the
  // others get them put in front (normalization), behind the AUD
  EXPECT_LT(1, key_frames_with_inserted_parameter_sets);
}

// the frames (by number) whose first NALUs are AUD + VPS
std::vector<std::size_t>
frames_with_parameter_sets(std::vector<std::vector<int>> const &frames) {
  std::vector<std::size_t> numbers;
  for (std::size_t idx = 0; idx < frames.size(); ++idx)
    if ((frames[idx].size() > 1) && (frames[idx][1] == mtx::hevc::NALU_TYPE_VIDEO_PARAM))
      numbers.push_back(idx);
  return numbers;
}

TEST(HEVCParameterSets, LeftAsTheyAreWhileTheyEqualTheConfigurationRecord) {
  mtx::hevc::es_parser_c parser;
  parser.set_add_changed_parameter_sets(true);
  auto frames = parse(parser, s_hevc, true);

  ASSERT_EQ(25u, frames.size());
  for (auto const &types : frames)
    EXPECT_EQ(mtx::hevc::NALU_TYPE_ACCESS_UNIT, types.front());

  // as in the stream: parameter sets in front of the first frame only
  EXPECT_EQ(std::vector<std::size_t>{ 0 }, frames_with_parameter_sets(frames));
}

TEST(HEVCParameterSets, PutInFrontOfKeyFramesWhileTheyDifferFromTheConfigurationRecord) {
  // two different encodes (25 frames each, a key frame every 6), each with
  // its parameter sets in front of its first frame only, one after the other
  mtx::hevc::es_parser_c parser;
  parser.set_add_changed_parameter_sets(true);
  auto frames = parse(parser, "tests/unit/data/video/two-encodes-headers-once.hevc", true);

  ASSERT_EQ(50u, frames.size());
  for (auto const &types : frames) {
    ASSERT_FALSE(types.empty());
    EXPECT_EQ(mtx::hevc::NALU_TYPE_ACCESS_UNIT, types.front());
    // only parameter sets are added, no SEI from the configuration record
    EXPECT_GE(1, std::count(types.begin(), types.end(), mtx::hevc::NALU_TYPE_PREFIX_SEI));
  }

  // the first encode as it is; every key frame of the second one (its
  // parameter sets differ from the configuration record, made from the first
  // encode's) with them in front, so that decoding can start there
  EXPECT_EQ((std::vector<std::size_t>{ 0, 25, 31, 37, 43, 49 }), frames_with_parameter_sets(frames));
}

}
