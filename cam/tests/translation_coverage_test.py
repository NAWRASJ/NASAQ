# SPDX-License-Identifier: GPL-3.0-or-later
"""Guard Arabic UI coverage without launching the player or touching its profile."""
from pathlib import Path
import collections
import json
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
CATALOG = (ROOT / "cam/src/nasaqlanguage.h").read_text(encoding="utf-8")
CATALOG += "\n".join(p.read_text(encoding="utf-8") for p in (ROOT / "cam/src").glob("nasaq*translations.inc"))
LITERAL = r'("(?:[^"\\]|\\.)*")'
entries = re.findall(r'\{QStringLiteral\(' + LITERAL + r'\),\s*\{QStringLiteral\(' + LITERAL + r'\),\s*QStringLiteral\(' + LITERAL + r'\)\}\}', CATALOG)
translations = {}
for source, english, arabic in entries:
    source, english, arabic = map(json.loads, (source, english, arabic))
    assert source not in translations, f"Duplicate translation: {source}"
    assert sorted(re.findall(r"%[1-9n]", source)) == sorted(re.findall(r"%[1-9n]", arabic)), source
    translations[source] = arabic
# Product/protocol names and formatting tokens must remain unchanged.
UNCHANGED = set("AAC|ASF (WMA)|AcoustID|CDDA|Discord|FLAC|Jellyfin|Last.fm|Listenbrainz|MP3|Opus|Plex|Qobuz|Radio Paradise|SomaFM|Speex|Spotify|Subsonic|Tidal|Vorbis|WavPack|hw|plughw|pcm|0px|%filename%|%originalyear%|%rating%|%url%|Form|MenuPopupToolButton|نسق NASAQ".split("|"))

def covered(text):
    key = text.replace("&", "")
    return (not re.search("[A-Za-z]", key) or key in translations or key in UNCHANGED
            or re.fullmatch(r"(?:Ctrl\+.*|F[1-9][0-9]?)", key))

missing = []
forms = list((ROOT / "src").rglob("*.ui"))
for path in forms:
    for node in ET.parse(path).iter("string"):
        if node.get("notr") != "true" and not covered(node.text or ""):
            missing.append((str(path.relative_to(ROOT)), node.text))
for folder in ("settings", "dialogs"):
    for path in (ROOT / "src" / folder).glob("*.cpp"):
        for literal in re.findall(r"\btr\(" + LITERAL, path.read_text(encoding="utf-8")):
            text = json.loads(literal)
            if not covered(text):
                missing.append((str(path.relative_to(ROOT)), text))
assert not missing, json.dumps(missing, ensure_ascii=False, indent=2)
print(f"PASS: {len(forms)} UI forms, settings/dialog source strings, {len(translations)} catalog entries; placeholders preserved.")
