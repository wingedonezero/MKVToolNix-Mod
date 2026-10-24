/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks from Matroska files into other files

*/

#include "common/common_pch.h"

#include "common/endian.h"
#include "common/hacks.h"
#include "common/hevc/hevcc.h"
#include "common/hevc/util.h"
#include "common/list_utils.h"
#include "extract/xtr_hevc.h"

xtr_hevc_c::xtr_hevc_c(std::string const &codec_id,
                       int64_t tid,
                       track_spec_t &tspec)
  : xtr_avc_c{codec_id, tid, tspec}
  , m_normalize_parameter_sets{mtx::hacks::is_engaged(mtx::hacks::NORMALIZE_PARAMETER_SETS)}
{
}

void
xtr_hevc_c::create_file(xtr_base_c *master,
                        libmatroska::KaxTrackEntry &track) {
  xtr_base_c::create_file(master, track);

  auto priv = find_child<libmatroska::KaxCodecPrivate>(&track);
  if (!priv)
    mxerror(fmt::format(FY("Track {0} with the CodecID '{1}' is missing the \"codec private\" element and cannot be extracted.\n"), m_tid, m_codec_id));

  m_decoded_codec_private = decode_codec_private(priv);
  if (m_decoded_codec_private->get_size() < 23)
    mxerror(fmt::format(FY("Track {0} CodecPrivate is too small.\n"), m_tid));

  m_parser.set_normalize_parameter_sets(m_normalize_parameter_sets);
  m_parser.set_add_changed_parameter_sets(!mtx::hacks::is_engaged(mtx::hacks::DONT_NORMALIZE_PARAMETER_SETS));
  m_parser.set_configuration_record(m_decoded_codec_private);

  m_nal_size_size = 1 + (m_decoded_codec_private->get_buffer()[21] & 3);

  // CodecPrivate's NALUs are written with the first frame (behind its
  // access unit delimiter, if any, and only those it does not carry itself)
  auto hevcc = mtx::hevc::hevcc_c::unpack(m_decoded_codec_private);
  for (auto const *list : { &hevcc.m_vps_list, &hevcc.m_sps_list, &hevcc.m_pps_list, &hevcc.m_sei_list })
    for (auto const &nalu : *list)
      m_codec_private_nalus.push_back(nalu);
}

bool
xtr_hevc_c::write_nal(uint8_t *data,
                      size_t &pos,
                      size_t data_size,
                      size_t write_nal_size_size) {
  if ((pos + write_nal_size_size) > data_size)
    return false;

  auto nal_size  = get_uint_be(&data[pos], write_nal_size_size);
  pos           += write_nal_size_size;

  if ((pos + nal_size) > data_size) {
    mxwarn(fmt::format(FY("Track {0}: NAL too big. Size according to header field: {1}, available bytes in packet: {2}. This NAL is defect and will be skipped.\n"), m_tid, nal_size, data_size - pos));
    return false;
  }

  auto nal_unit_type   = (data[pos] >> 1) & 0x3f;
  auto start_code_size = m_first_nalu || mtx::included_in(nal_unit_type, mtx::hevc::NALU_TYPE_VIDEO_PARAM, mtx::hevc::NALU_TYPE_SEQ_PARAM, mtx::hevc::NALU_TYPE_PIC_PARAM) ? 4 : 3;
  m_first_nalu         = false;

  m_out->write(ms_start_code + (4 - start_code_size), start_code_size);
  m_out->write(data + pos, nal_size);

  pos += nal_size;

  return true;
}

void
xtr_hevc_c::handle_frame(xtr_frame_t &f) {
  if (f.keyframe)
    m_parser.set_next_i_slice_is_key_frame();

  m_parser.add_bytes_framed(f.frame, m_nal_size_size);

  flush_frames();
}

void
xtr_hevc_c::flush_frames() {
  while (m_parser.frame_available()) {
    std::size_t pos{};

    auto frame  = m_parser.get_frame();
    auto buffer = frame.m_data->get_buffer();
    auto size   = frame.m_data->get_size();

    if (m_first_frame) {
      m_first_frame = false;
      write_first_frame(buffer, size);
      continue;
    }

    while (pos < size)
      write_nal(buffer, pos, size, m_nal_size_size);
  }
}

void
xtr_hevc_c::write_first_frame(uint8_t *buffer,
                              std::size_t size) {
  // the frame's NALUs (with the parser's NALU size length in front)
  auto const size_length = static_cast<std::size_t>(m_parser.get_nalu_size_length());
  std::vector<std::pair<std::size_t, std::size_t>> nalus;
  for (std::size_t pos = 0; (pos + size_length) <= size; ) {
    auto nalu_size = get_uint_be(&buffer[pos], size_length);
    if ((pos + size_length + nalu_size) > size)
      break;
    nalus.emplace_back(pos + size_length, nalu_size);
    pos += size_length + nalu_size;
  }

  std::vector<memory_cptr> missing;
  for (auto const &nalu : m_codec_private_nalus) {
    auto in_frame = std::any_of(nalus.begin(), nalus.end(), [buffer, &nalu](auto const &n) {
      return (n.second == nalu->get_size()) && !std::memcmp(&buffer[n.first], nalu->get_buffer(), n.second);
    });
    if (!in_frame)
      missing.push_back(nalu);
  }

  std::size_t pos = 0;

  // an access unit delimiter is the access unit's first NALU
  if (!nalus.empty() && (((buffer[nalus.front().first] >> 1) & 0x3f) == mtx::hevc::NALU_TYPE_ACCESS_UNIT))
    write_nal(buffer, pos, size, size_length);

  for (auto const &nalu : missing) {
    m_out->write(ms_start_code, 4);
    m_out->write(nalu);
    m_first_nalu = false;
  }

  while (pos < size)
    if (!write_nal(buffer, pos, size, size_length))
      break;
}

void
xtr_hevc_c::finish_track() {
  m_parser.flush();
  flush_frames();
}
