/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts VC-1 (Advanced Profile) video tracks into elementary streams
*/

#include "common/common_pch.h"

#include "common/ebml.h"
#include "common/endian.h"
#include "common/vc1.h"
#include "extract/xtr_vc1.h"

// A VC-1 Advanced Profile track (V_MS/VFW/FOURCC, FourCC WVC1) written as
// the elementary stream it came from (SMPTE 421M Annex E, with start codes):
// the sequence header and entry point from CodecPrivate in front unless the
// first frame carries them itself (mkvmerge keeps them in the frames), then
// the frames; a frame stored without its start code gets the frame start
// code back.

xtr_vc1_c::xtr_vc1_c(const std::string &codec_id,
                     int64_t tid,
                     track_spec_t &tspec)
  : xtr_base_c(codec_id, tid, tspec)
{
}

void
xtr_vc1_c::create_file(xtr_base_c *master,
                       libmatroska::KaxTrackEntry &track) {
  xtr_base_c::create_file(master, track);

  auto priv = find_child<libmatroska::KaxCodecPrivate>(&track);
  if (priv)
    m_headers = mtx::vc1::headers_from_vfw_codec_private(*decode_codec_private(priv));
}

void
xtr_vc1_c::handle_frame(xtr_frame_t &f) {
  auto buf  = f.frame->get_buffer();
  auto size = f.frame->get_size();

  if (m_first_frame) {
    m_first_frame = false;

    if (m_headers && !((4 <= size) && (mtx::vc1::MARKER_SEQHDR == get_uint32_be(buf))))
      m_out->write(m_headers);
  }

  if (!mtx::vc1::starts_with_start_code(buf, size)) {
    uint8_t frame_start_code[4];
    put_uint32_be(frame_start_code, mtx::vc1::MARKER_FRAME);
    m_out->write(frame_start_code, 4);
  }

  m_out->write(buf, size);
}
