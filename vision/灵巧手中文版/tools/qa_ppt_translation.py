from __future__ import annotations

import csv
import json
import re
import sys
import zipfile
from pathlib import Path

from pptx import Presentation
from lxml import etree


CJK_RE = re.compile(r"[\u3400-\u9fff]")
REPEATED_WORD_RE = re.compile(r"\b([A-Za-z][A-Za-z-]*)\b(?:\s+\1\b){2,}", re.I)
MARKER_RE = re.compile(r"X\s*Q\s*Z|Z\s*X\s*Q|⁇")
TARGET_PART_PREFIXES = (
    "ppt/slides/",
    "ppt/notesSlides/",
    "ppt/slideLayouts/",
    "ppt/slideMasters/",
    "ppt/charts/",
    "ppt/diagrams/",
    "ppt/comments/",
)


def scan_deck(path: Path):
    with zipfile.ZipFile(path) as zf:
        bad_member = zf.testzip()
        remaining = []
        for name in zf.namelist():
            if not name.endswith(".xml") or not name.startswith(TARGET_PART_PREFIXES):
                continue
            try:
                root = etree.fromstring(zf.read(name))
            except etree.XMLSyntaxError:
                continue
            chinese_nodes = [node.text for node in root.iter() if node.text and CJK_RE.search(node.text)]
            if chinese_nodes:
                remaining.append({"part": name, "texts": chinese_nodes[:10]})
    prs = Presentation(path)
    return {
        "file": path.name,
        "slides": len(prs.slides),
        "size_bytes": path.stat().st_size,
        "zip_crc_error": bad_member,
        "parts_with_chinese": remaining,
    }


def scan_report(path: Path):
    suspicious = []
    with path.open(encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f, delimiter="\t")
        for row in reader:
            source = row["source"]
            translation = row["translation"]
            reasons = []
            if CJK_RE.search(translation):
                reasons.append("remaining_chinese")
            if MARKER_RE.search(translation):
                reasons.append("marker")
            if REPEATED_WORD_RE.search(translation):
                reasons.append("word_repetition")
            src_len = max(1, len(source.strip()))
            if len(translation.strip()) > max(120, src_len * 8):
                reasons.append("abnormally_long")
            if not translation.strip():
                reasons.append("empty")
            if reasons:
                suspicious.append({**row, "reasons": ",".join(reasons)})
    return suspicious


def main(output_dir: Path, report_path: Path, qa_dir: Path):
    qa_dir.mkdir(parents=True, exist_ok=True)
    pptx_paths = [p for p in output_dir.glob("*.pptx") if not p.name.startswith("~$")]
    decks = [scan_deck(p) for p in sorted(pptx_paths, key=lambda p: p.name.casefold())]
    suspicious = scan_report(report_path)
    (qa_dir / "deck_validation.json").write_text(
        json.dumps(decks, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    with (qa_dir / "suspicious_translations.tsv").open("w", encoding="utf-8", newline="") as f:
        fields = ["count", "source", "translation", "reasons"]
        writer = csv.DictWriter(f, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        writer.writerows(suspicious)
    print(json.dumps({"decks": decks, "suspicious_count": len(suspicious)}, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise SystemExit("usage: qa_ppt_translation.py OUTPUT_DIR REPORT_TSV QA_DIR")
    main(Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3]))
