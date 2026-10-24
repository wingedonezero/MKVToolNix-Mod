#include "common/common_pch.h"

#include <fstream>

#include "common/avc/es_parser.h"

#include "tests/unit/init.h"

namespace {

// Two small soft-pulldown streams (24 frames, B frames, a picture timing SEI
// message per frame, one tick = one field at 59.94):
// - 3:2: pic_struct 5, 4, 6, 3 repeated: 12 frames of three fields, 12 of
//   two, 60 fields;
// - mixed: 8 frames of two fields (pic_struct 3, 4), then 5, 4, 6, 3; the
//   frames 12 and 13 (stream order) without the message: 55 fields.
std::string const s_pulldown_3_2 = "tests/unit/data/video/soft-pulldown-3-2.h264";
std::string const s_pulldown_mix = "tests/unit/data/video/soft-pulldown-mixed-missing.h264";

int64_t const s_field_num = 1001, s_field_den = 60000;

std::vector<mtx::xyzvc::frame_t>
parse(mtx::avc::es_parser_c &parser,
      std::string const &file_name) {
  std::ifstream in{file_name, std::ios::binary};
  std::vector<uint8_t> data{ std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{} };
  EXPECT_FALSE(data.empty());

  parser.add_bytes(data.data(), data.size());
  parser.flush();

  std::vector<mtx::xyzvc::frame_t> frames;
  while (parser.frame_available())
    frames.push_back(parser.get_frame());

  std::sort(frames.begin(), frames.end(), [](auto const &a, auto const &b) { return a.m_start < b.m_start; });

  return frames;
}

// the time of `fields` fields, rounded to the nearest ns
int64_t
fields_ns(int64_t fields) {
  return mtx::to_int_rounded(mtx::rational(fields * s_field_num, s_field_den) * 1'000'000'000);
}

// every frame starts where the one before ends, on the field grid; returns
// the fields per frame
std::vector<int64_t>
fields_per_frame(std::vector<mtx::xyzvc::frame_t> const &frames) {
  std::vector<int64_t> result;
  auto fields = int64_t{};

  for (auto const &frame : frames) {
    EXPECT_EQ(fields_ns(fields), frame.m_start);

    auto length = mtx::to_int_rounded(mtx::rational(frame.m_end - frame.m_start, 1) * s_field_den / (s_field_num * 1'000'000'000ll));
    EXPECT_EQ(fields_ns(fields + length), frame.m_end);

    result.push_back(length);
    fields += length;
  }

  return result;
}

TEST(AVCPicStruct, SoftPulldownFieldsCounted) {
  mtx::avc::es_parser_c parser;
  auto frames = parse(parser, s_pulldown_3_2);

  ASSERT_EQ(24u, frames.size());
  auto fields = fields_per_frame(frames);

  EXPECT_EQ(12, std::count(fields.begin(), fields.end(), 3));
  EXPECT_EQ(12, std::count(fields.begin(), fields.end(), 2));
  EXPECT_EQ(fields_ns(60), frames.back().m_end);        // 1.001 s
  EXPECT_EQ(0,             parser.get_num_frames_without_picture_timing());
}

TEST(AVCPicStruct, MixedCadenceAndFramesWithoutTheMessage) {
  mtx::avc::es_parser_c parser;
  auto frames = parse(parser, s_pulldown_mix);

  ASSERT_EQ(24u, frames.size());
  auto fields = fields_per_frame(frames);

  // 8 + 8 + 1 frames of two fields (the two without the message among them), 7 of three
  EXPECT_EQ(7,             std::count(fields.begin(), fields.end(), 3));
  EXPECT_EQ(17,            std::count(fields.begin(), fields.end(), 2));
  EXPECT_EQ(fields_ns(55), frames.back().m_end);
  EXPECT_EQ(2,             parser.get_num_frames_without_picture_timing());
}

}
