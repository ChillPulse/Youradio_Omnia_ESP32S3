import sys
from pathlib import Path

p = Path("yoRadio/src/audioI2S/Audio.cpp")
txt = p.read_text(encoding="utf-8", errors="replace")

# 1) EOF sentinel must exist and be the last non-empty line
lines = [l.rstrip("\n") for l in txt.splitlines()]
nonempty = [l.strip() for l in lines if l.strip() != ""]
if not nonempty:
    print("FAIL: Audio.cpp is empty")
    sys.exit(2)

if nonempty[-1] != "// === AUDIO.CPP EOF (DO NOT ADD CODE BELOW) ===":
    print("FAIL: Missing/incorrect EOF sentinel line")
    sys.exit(2)

# 2) No raw AI garbage tokens outside comments (hard fail)
# This string previously broke builds (Log69)
if "ack proportional if no index" in txt:
    print("FAIL: Found raw 'ack proportional if no index' in Audio.cpp")
    sys.exit(2)

# 3) Ensure function exists exactly once
needle = "bool Audio::omnia_aacSeekMs(uint32_t ms)"
cnt = txt.count(needle)
if cnt != 1:
    print(f"FAIL: Expected exactly 1 occurrence of '{needle}', got {cnt}")
    sys.exit(2)

# 4) Ensure there is no code after the final closing brace of the file
last_brace = txt.rfind("}")
tail = txt[last_brace + 1 :]
# allow whitespace and the EOF sentinel comment only
tail_stripped = "\n".join([l.strip() for l in tail.splitlines() if l.strip() != ""])
if tail_stripped not in ["// === AUDIO.CPP EOF (DO NOT ADD CODE BELOW) ==="]:
    print("FAIL: There is unexpected content after last '}' in Audio.cpp")
    sys.exit(2)

print("OK: Audio.cpp sanity checks passed")
sys.exit(0)
