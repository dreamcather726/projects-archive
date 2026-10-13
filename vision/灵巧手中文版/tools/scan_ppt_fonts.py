from __future__ import annotations

import json
import sys
import zipfile
from collections import Counter
from pathlib import Path

from lxml import etree


A_NS = "http://schemas.openxmlformats.org/drawingml/2006/main"
NS = {"a": A_NS}


def local_name(name: str) -> str:
    return name.rsplit("}", 1)[-1]


def scan(path: Path) -> dict:
    declaration_counts = Counter()
    harmony_parts = []
    run_font_sets = Counter()
    with zipfile.ZipFile(path) as zf:
        for name in zf.namelist():
            if not name.startswith("ppt/") or not name.endswith(".xml"):
                continue
            data = zf.read(name)
            if b"HarmonyOS" in data:
                harmony_parts.append(name)
            try:
                root = etree.fromstring(data)
            except etree.XMLSyntaxError:
                continue
            for node in root.iter():
                for attr_name, value in node.attrib.items():
                    if local_name(attr_name).lower() in {"typeface", "font", "fontfamily", "font-family"}:
                        declaration_counts[f"{local_name(node.tag)}:{local_name(attr_name)}={value}"] += 1
            for container in root.xpath(".//a:r[a:t] | .//a:fld[a:t]", namespaces=NS):
                text_node = container.find("a:t", NS)
                if text_node is None or not (text_node.text or ""):
                    continue
                properties = container.find("a:rPr", NS)
                values = []
                for font_tag in ("latin", "ea", "cs"):
                    font_node = properties.find(f"a:{font_tag}", NS) if properties is not None else None
                    values.append(font_node.get("typeface") if font_node is not None else None)
                run_font_sets[str(tuple(values))] += 1
    return {
        "file": path.name,
        "harmony_parts": harmony_parts,
        "run_font_sets": dict(run_font_sets),
        "font_declarations": dict(declaration_counts),
    }


def main(input_dir: Path, report: Path) -> None:
    paths = sorted(
        [p for p in input_dir.glob("*.pptx") if not p.name.startswith("~$")],
        key=lambda p: p.name.casefold(),
    )
    results = [scan(path) for path in paths]
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding="utf-8")
    summary = [
        {
            "file": item["file"],
            "harmony_parts": len(item["harmony_parts"]),
            "run_font_sets": item["run_font_sets"],
        }
        for item in results
    ]
    print(json.dumps(summary, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: scan_ppt_fonts.py INPUT_DIR REPORT_JSON")
    main(Path(sys.argv[1]), Path(sys.argv[2]))
