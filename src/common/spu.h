/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper functions for SPU data (SubPicture Unist — subtitles on DVDs)

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/timestamp.h"

namespace mtx::spu {

// HD DVD units start with a zero 16-bit word, then the unit's size and the
// offset of its first control sequence as 32-bit values (DVD: 16 bits each).
bool is_hd_dvd_unit(uint8_t const *data, std::size_t const buf_size);
timestamp_c get_duration(uint8_t const *data, std::size_t const buf_size);
void set_duration(uint8_t *data, std::size_t const buf_size, timestamp_c const &duration);

}
