from __future__ import annotations

import json
import sys
import zipfile
from pathlib import Path

from lxml import etree
from pptx import Presentation


A_NS = "http://schemas.openxmlformats.org/drawingml/2006/main"
NS = {"a": A_NS}
FONT_NAME = "Times New Roman"


def scan(path: Path) -> dict:
    text_runs = 0
    missing_or_wrong = []
    wrong_latin_declarations = []
    with zipfile.ZipFile(path) as zf:
        crc_error = zf.testzip()
        for name in zf.namelist():
            if not name.startswith("ppt/") or not name.endswith(".xml"):
                continue
            try:
                root = etree.fromstring(zf.read(name))
            except etree.XMLSyntaxError:
                continue
            for font_node in root.xpath(
                ".//a:latin | .//a:ea | .//a:cs | .//a:font[@typeface] | .//a:sym[contains(@typeface, 'HarmonyOS')]",
                namespaces=NS,
            ):
                if font_node.get("typeface") != FONT_NAME:
                    wrong_latin_declarations.append(
                        {
                            "part": name,
                            "kind": etree.QName(font_node).localname,
                            "font": font_node.get("typeface"),
                        }
                    )
            for container in root.xpath(".//a:r[a:t] | .//a:fld[a:t]", namespaces=NS):
                text_node = container.find("a:t", NS)
                if text_node is None or not (text_node.text or ""):
                    continue
                text_runs += 1
                properties = container.find("a:rPr", NS)
                font_nodes = [
                    properties.find(f"a:{font_tag}", NS) if properties is not None else None
                    for font_tag in ("latin", "ea", "cs")
                ]
                if any(node is None or node.get("typeface") != FONT_NAME for node in font_nodes):
                    missing_or_wrong.append({"part": name, "text": (text_node.text or "")[:80]})
    presentation = Presentation(path)
    return {
        "file": path.name,
        "slides": len(presentation.slides),
        "text_runs": text_runs,
        "zip_crc_error": crc_error,
        "noncompliant_text_runs": missing_or_wrong,
        "wrong_latin_declarations": wrong_latin_declarations,
    }


def main(input_dir: Path, report_path: Path) -> None:
    paths = sorted(
        [p for p in input_dir.glob("*.pptx") if not p.name.startswith("~$")],
        key=lambda p: p.name.casefold(),
    )
    report = [scan(path) for path in paths]
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    summary = [
        {
            "file": item["file"],
            "slides": item["slides"],
            "text_runs": item["text_runs"],
            "zip_crc_error": item["zip_crc_error"],
            "noncompliant_text_runs": len(item["noncompliant_text_runs"]),
            "wrong_latin_declarations": len(item["wrong_latin_declarations"]),
        }
        for item in report
    ]
    print(json.dumps(summary, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: qa_ppt_font.py INPUT_DIR REPORT_JSON")
    main(Path(sys.argv[1]), Path(sys.argv[2]))
