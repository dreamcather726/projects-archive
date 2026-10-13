from __future__ import annotations

import argparse
import json
import zipfile
from pathlib import Path

from lxml import etree


A_NS = "http://schemas.openxmlformats.org/drawingml/2006/main"
NS = {"a": A_NS}
FONT_NAME = "Times New Roman"


def qn(local: str) -> str:
    return f"{{{A_NS}}}{local}"


def ensure_font_set(properties: etree._Element) -> tuple[int, int]:
    inserted = 0
    updated = 0
    schema_order = ["latin", "ea", "cs"]
    for font_tag in schema_order:
        node = properties.find(f"a:{font_tag}", NS)
        if node is None:
            node = etree.Element(qn(font_tag))
            # Keep DrawingML schema order: latin, ea, cs, sym.
            desired_rank = schema_order.index(font_tag)
            insert_at = len(properties)
            for index, child in enumerate(properties):
                child_name = etree.QName(child).localname
                if child_name == "sym":
                    insert_at = index
                    break
                if child_name in schema_order and schema_order.index(child_name) > desired_rank:
                    insert_at = index
                    break
            properties.insert(insert_at, node)
            inserted += 1
        if node.get("typeface") != FONT_NAME:
            node.set("typeface", FONT_NAME)
            updated += 1
    return inserted, updated


def update_xml(data: bytes) -> tuple[bytes, dict[str, int]]:
    root = etree.fromstring(data)
    inserted = 0
    updated = 0
    run_count = 0

    # Set all existing Latin/East-Asian/complex-script declarations. PowerPoint
    # can prefer ea/cs for English text when a run retains a zh-CN language tag.
    for font_node in root.xpath(".//a:latin | .//a:ea | .//a:cs", namespaces=NS):
        if font_node.get("typeface") != FONT_NAME:
            font_node.set("typeface", FONT_NAME)
            updated += 1

    # Remove HarmonyOS retained in symbol-font slots on ordinary text runs.
    # Preserve genuine symbol fonts such as Wingdings to avoid breaking icons.
    for font_node in root.xpath(".//a:sym[@typeface]", namespaces=NS):
        if "HarmonyOS" in (font_node.get("typeface") or ""):
            font_node.set("typeface", FONT_NAME)
            updated += 1

    # Theme supplemental mappings (for example Hans -> HarmonyOS Sans SC Black)
    # can override a run after PowerPoint normalizes it. Replace those mappings too.
    for font_node in root.xpath(".//a:font[@typeface]", namespaces=NS):
        if font_node.get("typeface") != FONT_NAME:
            font_node.set("typeface", FONT_NAME)
            updated += 1

    # Make each visible text run explicit so local formatting cannot override the theme.
    containers = root.xpath(".//a:r[a:t] | .//a:fld[a:t]", namespaces=NS)
    for container in containers:
        text_node = container.find("a:t", NS)
        if text_node is None or not (text_node.text or ""):
            continue
        run_count += 1
        properties = container.find("a:rPr", NS)
        if properties is None:
            properties = etree.Element(qn("rPr"))
            container.insert(0, properties)
        add_count, update_count = ensure_font_set(properties)
        inserted += add_count
        updated += update_count

    # Paragraph defaults and end-paragraph properties affect bullets and empty runs.
    for properties in root.xpath(".//a:defRPr | .//a:endParaRPr", namespaces=NS):
        add_count, update_count = ensure_font_set(properties)
        inserted += add_count
        updated += update_count

    changed = inserted > 0 or updated > 0
    if not changed:
        return data, {"runs": run_count, "inserted": 0, "updated": 0}
    output = etree.tostring(
        root,
        encoding="UTF-8",
        xml_declaration=True,
        standalone=True,
    )
    return output, {"runs": run_count, "inserted": inserted, "updated": updated}


def process_pptx(source: Path, destination: Path) -> dict:
    destination.parent.mkdir(parents=True, exist_ok=True)
    totals = {"runs": 0, "inserted": 0, "updated": 0, "xml_parts_changed": 0}
    with zipfile.ZipFile(source, "r") as zin, zipfile.ZipFile(destination, "w") as zout:
        for info in zin.infolist():
            data = zin.read(info.filename)
            if info.filename.startswith("ppt/") and info.filename.endswith(".xml"):
                try:
                    new_data, stats = update_xml(data)
                except etree.XMLSyntaxError:
                    new_data = data
                    stats = {"runs": 0, "inserted": 0, "updated": 0}
                if new_data != data:
                    totals["xml_parts_changed"] += 1
                data = new_data
                for key in ("runs", "inserted", "updated"):
                    totals[key] += stats[key]
            zout.writestr(info, data)
    return totals


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("input_dir", type=Path)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("--report", type=Path, default=Path("work/times_new_roman_report.json"))
    args = parser.parse_args()

    sources = sorted(
        [p for p in args.input_dir.glob("*.pptx") if not p.name.startswith("~$")],
        key=lambda p: p.name.casefold(),
    )
    if not sources:
        raise SystemExit("No PPTX files found")

    report = []
    for source in sources:
        destination = args.output_dir / f"{source.stem}_TimesNewRoman.pptx"
        stats = process_pptx(source, destination)
        item = {"source": source.name, "output": destination.name, **stats}
        report.append(item)
        print(json.dumps(item, ensure_ascii=False), flush=True)

    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
