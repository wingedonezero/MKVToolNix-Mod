#include "common/common_pch.h"

#include <matroska/KaxChapters.h>

#include "common/chapters/chapters.h"
#include "common/ebml.h"

#include "tests/unit/init.h"

namespace {

// One edition with a chapter at each start (ns).
mtx::chapters::kax_cptr
make_chapters(std::vector<int64_t> const &starts) {
  auto chapters = std::make_shared<libmatroska::KaxChapters>();
  auto edition  = new libmatroska::KaxEditionEntry;
  chapters->PushElement(*edition);

  for (auto start : starts) {
    auto atom = new libmatroska::KaxChapterAtom;
    get_child<libmatroska::KaxChapterTimeStart>(*atom).SetValue(start);
    edition->PushElement(*atom);
  }

  return chapters;
}

// The start (ns) of every chapter left.
std::vector<int64_t>
starts_of(libmatroska::KaxChapters &chapters) {
  std::vector<int64_t> starts;

  for (auto element : chapters) {
    auto edition = dynamic_cast<libmatroska::KaxEditionEntry *>(element);
    if (!edition)
      continue;

    for (auto child : *edition)
      if (auto atom = dynamic_cast<libmatroska::KaxChapterAtom *>(child); atom)
        starts.push_back(mtx::chapters::get_start(*atom));
  }

  return starts;
}

// A split part starting with the key frame stored at 1,372,204 ms whose
// chapter starts at 1,372,204.166667 ms (the frame's exact time).
int64_t const s_part_start = 1'372'204'000'000;
int64_t const s_chapter    = 1'372'204'166'667;

TEST(ChaptersSelectInTimeframe, WithoutToleranceTheChapterBeforeSpansIntoThePart) {
  auto chapters = make_chapters({ 1'280'070'458'333, s_chapter, 1'418'250'166'667 });

  ASSERT_TRUE(mtx::chapters::select_in_timeframe(chapters.get(), s_part_start, -1, s_part_start));
  EXPECT_EQ((std::vector<int64_t>{ 0, 166'667, 46'046'166'667 }), starts_of(*chapters));
}

TEST(ChaptersSelectInTimeframe, AChapterLessThanTheToleranceAfterTheStartStartsThePart) {
  auto chapters = make_chapters({ 1'280'070'458'333, s_chapter, 1'418'250'166'667 });

  ASSERT_TRUE(mtx::chapters::select_in_timeframe(chapters.get(), s_part_start, -1, s_part_start, 1'000'000));
  EXPECT_EQ((std::vector<int64_t>{ 0, 46'046'166'667 }), starts_of(*chapters));
}

TEST(ChaptersSelectInTimeframe, AChapterEndingLessThanTheToleranceIntoThePartIsLeftOut) {
  // the chapter before ends with an end timestamp of its own
  auto chapters = make_chapters({ 1'280'070'458'333, s_chapter + 1'000'000 });
  auto edition  = find_child<libmatroska::KaxEditionEntry>(*chapters);
  auto first    = find_child<libmatroska::KaxChapterAtom>(*edition);
  get_child<libmatroska::KaxChapterTimeEnd>(*first).SetValue(s_chapter);

  ASSERT_TRUE(mtx::chapters::select_in_timeframe(chapters.get(), s_part_start, -1, s_part_start, 1'000'000));
  EXPECT_EQ((std::vector<int64_t>{ 1'166'667 }), starts_of(*chapters));
}

TEST(ChaptersSelectInTimeframe, AChapterReachingAWholeToleranceIntoThePartIsKept) {
  // split in the middle of a chapter: it still starts the part, as before
  auto chapters = make_chapters({ 1'280'070'458'333, s_part_start + 1'000'000, 1'418'250'166'667 });

  ASSERT_TRUE(mtx::chapters::select_in_timeframe(chapters.get(), s_part_start, -1, s_part_start, 1'000'000));
  EXPECT_EQ((std::vector<int64_t>{ 0, 1'000'000, 46'046'166'667 }), starts_of(*chapters));
}

TEST(ChaptersSelectInTimeframe, ChaptersOnTheSplitPointAreUnchanged) {
  auto with    = make_chapters({ 0, s_part_start, 1'418'250'000'000 });
  auto without = make_chapters({ 0, s_part_start, 1'418'250'000'000 });

  ASSERT_TRUE(mtx::chapters::select_in_timeframe(with.get(),    s_part_start, -1, s_part_start, 1'000'000));
  ASSERT_TRUE(mtx::chapters::select_in_timeframe(without.get(), s_part_start, -1, s_part_start));
  EXPECT_EQ(starts_of(*without), starts_of(*with));
  EXPECT_EQ((std::vector<int64_t>{ 0, 46'046'000'000 }), starts_of(*with));
}

}
