#include "common/common_pch.h"

#include "common/endian.h"
#include "common/spu.h"

#include "tests/unit/init.h"

namespace {

// A sub-picture unit with two display control sequences: the first starts
// the display (with the commands given), the second (delay stop_delay)
// stops it and points to itself. hd_dvd: the HD DVD form (zero word, then
// the size and the control sequence offsets as 32 bits).
std::vector<uint8_t>
make_unit(bool hd_dvd,
          std::vector<uint8_t> const &start_commands,
          unsigned int stop_delay) {
  auto offset_size = hd_dvd ? 4u : 2u;
  auto header_size = hd_dvd ? 10u : 4u;
  auto first       = header_size + 4;                       // four bytes of "pixel data"
  auto second      = first + 2 + offset_size + start_commands.size() + 2;
  auto size        = second + 2 + offset_size + 2;

  std::vector<uint8_t> unit(size, 0x00);
  auto put_offset = [&](std::size_t pos, uint32_t value) {
    if (hd_dvd)
      put_uint32_be(&unit[pos], value);
    else
      put_uint16_be(&unit[pos], value);
  };

  if (hd_dvd) {
    put_uint32_be(&unit[2], size);
    put_uint32_be(&unit[6], first);
  } else {
    put_uint16_be(&unit[0], size);
    put_uint16_be(&unit[2], first);
  }

  for (auto i = header_size; i < first; ++i)
    unit[i] = 0x11;

  auto pos = first;
  put_uint16_be(&unit[pos], 0);
  put_offset(pos + 2, second);
  pos += 2 + offset_size;
  for (auto command : start_commands)
    unit[pos++] = command;
  unit[pos++] = 0x01;                                       // start display
  unit[pos++] = 0xff;

  put_uint16_be(&unit[pos], stop_delay);
  put_offset(pos + 2, second);
  pos += 2 + offset_size;
  unit[pos++] = 0x02;                                       // stop display
  unit[pos++] = 0xff;

  return unit;
}

std::vector<uint8_t>
hd_dvd_start_commands() {
  std::vector<uint8_t> commands;

  commands.push_back(0x83);                                 // palette: 256 x 3 bytes
  commands.insert(commands.end(), 768, 0x02);               // (bytes that look like commands)
  commands.push_back(0x84);                                 // contrast: 256 bytes
  commands.insert(commands.end(), 256, 0xff);
  commands.push_back(0x85);                                 // display area
  commands.insert(commands.end(), { 0x00, 0x07, 0x7f, 0x00, 0x04, 0x37 });
  commands.push_back(0x86);                                 // field addresses, 32 bits each
  commands.insert(commands.end(), { 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0c });

  return commands;
}

TEST(SPU, DetectsHDDVDUnits) {
  auto dvd    = make_unit(false, {}, 96);
  auto hd_dvd = make_unit(true,  {}, 96);

  EXPECT_FALSE(mtx::spu::is_hd_dvd_unit(dvd.data(),    dvd.size()));
  EXPECT_TRUE(mtx::spu::is_hd_dvd_unit(hd_dvd.data(),  hd_dvd.size()));
  EXPECT_FALSE(mtx::spu::is_hd_dvd_unit(hd_dvd.data(), 9));
}

TEST(SPU, DurationOfDVDUnit) {
  auto unit = make_unit(false, { 0x03, 0x01, 0x23, 0x04, 0xff, 0xf0 }, 96);

  EXPECT_EQ(timestamp_c::mpeg(96 * 1024), mtx::spu::get_duration(unit.data(), unit.size()));
}

TEST(SPU, DurationOfHDDVDUnit) {
  auto unit = make_unit(true, hd_dvd_start_commands(), 146);

  EXPECT_EQ(timestamp_c::mpeg(146 * 1024), mtx::spu::get_duration(unit.data(), unit.size()));
}

TEST(SPU, HDDVDCommandsAreUnknownInDVDUnits) {
  auto unit = make_unit(false, { 0x85, 0x00, 0x07, 0x7f, 0x00, 0x04, 0x37 }, 96);

  // An unknown command ends its control sequence; the next one still counts.
  EXPECT_EQ(timestamp_c::mpeg(96 * 1024), mtx::spu::get_duration(unit.data(), unit.size()));
}

TEST(SPU, SetDurationOfHDDVDUnit) {
  auto unit     = make_unit(true, hd_dvd_start_commands(), 146);
  auto expected = make_unit(true, hd_dvd_start_commands(), 198);

  // 198 x 1024 ticks: a whole number of ns, so the delay converts back exactly
  mtx::spu::set_duration(unit.data(), unit.size(), timestamp_c::mpeg(198 * 1024));

  EXPECT_EQ(expected, unit);
}

TEST(SPU, NoStopDisplayInTruncatedHDDVDUnit) {
  auto unit = make_unit(true, hd_dvd_start_commands(), 146);

  EXPECT_FALSE(mtx::spu::get_duration(unit.data(), 800).valid());
}

}
