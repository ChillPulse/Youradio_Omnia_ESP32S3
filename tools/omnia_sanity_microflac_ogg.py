import sys
from pathlib import Path

cpp = Path("yoRadio/src/audioI2S/micro_flac/flac_decoder.cpp")
h   = Path("yoRadio/src/audioI2S/micro_flac/flac_decoder.h")

txt = cpp.read_text(encoding="utf-8", errors="replace")
ht  = h.read_text(encoding="utf-8", errors="replace")

# Must have stash drain
need = [
    "ogg_stash_.empty()",
    "decode_native(s, s_len",
    "ogg_stash_.erase",
]
for n in need:
    if n not in txt:
        print(f"FAIL: missing '{n}' in flac_decoder.cpp")
        sys.exit(2)

# Must actually WRITE chunk/demux snapshots in flac_decoder.cpp (not only declare in .h)
for n in [
    "this->last_ogg_res_dbg_ =",
    "this->last_ogg_packet_len_dbg_ =",
    "this->last_chunk_len_ =",
    "this->last_chunk_off_ =",
    "this->last_chunk_prefix_valid_ =",
]:
    if n not in txt:
        print(f"FAIL: missing '{n}' assignment in flac_decoder.cpp")
        sys.exit(2)

# No duplicate demux snapshot block (prevents unsafe packet field reads when result != OGG_OK)
if txt.count("Log75/76: snapshot last demux result ALWAYS") != 1:
    print("FAIL: expected exactly 1 'snapshot last demux result ALWAYS' block in flac_decoder.cpp")
    sys.exit(2)

if "Snapshot last demux result (for Log76)" in txt:
    print("FAIL: duplicate demux snapshot block marker still present in flac_decoder.cpp")
    sys.exit(2)

# Must update end_of_packet tracking (from ogg demux state)
if "is_end_of_packet" not in txt:
    print("FAIL: missing 'is_end_of_packet' usage in flac_decoder.cpp")
    sys.exit(2)

# Public getter must exist (already added earlier, but enforce)
if "get_ogg_stash_len" not in ht:
    print("FAIL: missing get_ogg_stash_len() in flac_decoder.h")
    sys.exit(2)

# Native decode diag must exist
for n in ["get_last_native_result", "get_last_native_input_len", "get_last_native_bytes_index"]:
    if n not in ht:
        print(f"FAIL: missing {n}() in flac_decoder.h")
        sys.exit(2)

for n in ["last_native_result_", "last_native_input_len_", "last_native_bytes_index_"]:
    if n not in ht:
        print(f"FAIL: missing {n} field in flac_decoder.h")
        sys.exit(2)

for n in ["get_last_ogg_dbg_state", "get_last_ogg_dbg_assembling", "get_last_ogg_dbg_skipping",
          "get_last_ogg_dbg_packet_size", "get_last_ogg_dbg_buf_cap"]:
    if n not in ht:
        print(f"FAIL: missing {n}() in flac_decoder.h")
        sys.exit(2)

for n in ["get_last_ogg_body_prefix_valid", "get_last_ogg_body_len", "last_ogg_body_prefix_"]:
    if n not in ht:
        print(f"FAIL: missing {n} in flac_decoder.h")
        sys.exit(2)

ogg_h = Path("yoRadio/src/audioI2S/micro_flac/micro_ogg/ogg_demuxer.h").read_text(encoding="utf-8", errors="replace")
if "peak_buffer_capacity_{0}" not in ogg_h:
    print("FAIL: missing peak_buffer_capacity_{0} in ogg_demuxer.h")
    sys.exit(2)

for n in ["get_last_chunk_prefix_valid", "last_chunk_prefix_", "get_last_ogg_res", "last_ogg_res_dbg_"]:
    if n not in ht:
        print(f"FAIL: missing {n} in flac_decoder.h")
        sys.exit(2)

if "(k == 0 ? 1 : k)" in txt:
    print("FAIL: flac_decoder.cpp still contains '(k == 0 ? 1 : k)' (destroys OggS capture)")
    sys.exit(2)

if "bytes_consumed = 1;  // anti-stuck: guarantee window advance" in txt:
    print("FAIL: flac_decoder.cpp still forces bytes_consumed=1 on OGG_NEED_MORE_DATA (sync break risk)")
    sys.exit(2)

if "bytes_consumed > 0 && (mf_millis() - t0) > 40" in txt:
    print("FAIL: decode_ogg still uses progress-only time budget (can hang Log78)")
    sys.exit(2)

if "Prevent long byte-by-byte scanning of large packets" not in txt:
    print("FAIL: missing packet slicing time-budget guard")
    sys.exit(2)

# Ensure zero_cons diagnostics + adopt demux consume exist in Audio.cpp
a = Path("yoRadio/src/audioI2S/Audio.cpp").read_text(encoding="utf-8", errors="replace")
if "microflac adopt demux_cons=" not in a:
    print("FAIL: missing 'microflac adopt demux_cons=' in Audio.cpp")
    sys.exit(2)
if "microflac OGG_STATE(zero)" not in a:
    print("FAIL: missing 'microflac OGG_STATE(zero)' in Audio.cpp")
    sys.exit(2)
if "microflac CHUNK(zero)" not in a:
    print("FAIL: missing 'microflac CHUNK(zero)' in Audio.cpp")
    sys.exit(2)

ogg_h2 = Path("yoRadio/src/audioI2S/micro_flac/micro_ogg/ogg_demuxer.h").read_text(encoding="utf-8", errors="replace")
if "relaxed_stream_checks" not in ogg_h2:
    print("FAIL: missing relaxed_stream_checks in ogg_demuxer.h")
    sys.exit(2)
if "cfg.relaxed_stream_checks = true;" not in txt:
    print("FAIL: missing cfg.relaxed_stream_checks = true; in flac_decoder.cpp")
    sys.exit(2)
ogg_cpp = Path("yoRadio/src/audioI2S/micro_flac/micro_ogg/ogg_demuxer.cpp").read_text(encoding="utf-8", errors="replace")
if "config_.relaxed_stream_checks" not in ogg_cpp:
    print("FAIL: missing config_.relaxed_stream_checks in ogg_demuxer.cpp")
    sys.exit(2)

print("OK: microflac ogg stash/endpkt sanity checks passed")
sys.exit(0)
