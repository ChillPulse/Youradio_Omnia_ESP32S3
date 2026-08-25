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

print("OK: microflac ogg stash/endpkt sanity checks passed")
sys.exit(0)
