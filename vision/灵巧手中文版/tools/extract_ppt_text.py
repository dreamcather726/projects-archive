from __future__ import annotations

import json
import sys
from pathlib import Path

from pptx import Presentation
from pptx.enum.shapes import MSO_SHAPE_TYPE


def iter_shapes(shapes, prefix=""):
    for shape in shapes:
        path = f"{prefix}/{shape.shape_id}" if prefix else str(shape.shape_id)
        yield path, shape
        if shape.shape_type == MSO_SHAPE_TYPE.GROUP:
            yield from iter_shapes(shape.shapes, path)


def extract_text_frame(tf):
    items = []
    for p_idx, paragraph in enumerate(tf.paragraphs):
        if paragraph.runs:
            for r_idx, run in enumerate(paragraph.runs):
                if run.text:
                    items.append({"paragraph": p_idx, "run": r_idx, "text": run.text})
        elif paragraph.text:
            items.append({"paragraph": p_idx, "run": None, "text": paragraph.text})
    return items


def main(root: Path, output: Path):
    result = []
    for pptx_path in sorted(root.glob("*.pptx"), key=lambda p: p.name.casefold()):
        if pptx_path.name.startswith("~$") or "英文版" in pptx_path.stem:
            continue
        prs = Presentation(pptx_path)
        deck = {
            "file": pptx_path.name,
            "slides": len(prs.slides),
            "width": prs.slide_width,
            "height": prs.slide_height,
            "items": [],
        }
        for slide_idx, slide in enumerate(prs.slides, 1):
            for shape_path, shape in iter_shapes(slide.shapes):
                if getattr(shape, "has_text_frame", False):
                    for item in extract_text_frame(shape.text_frame):
                        deck["items"].append({
                            "slide": slide_idx,
                            "kind": "text",
                            "shape_path": shape_path,
                            **item,
                        })
                if getattr(shape, "has_table", False):
                    for row_idx, row in enumerate(shape.table.rows):
                        for col_idx, cell in enumerate(row.cells):
                            for item in extract_text_frame(cell.text_frame):
                                deck["items"].append({
                                    "slide": slide_idx,
                                    "kind": "table",
                                    "shape_path": shape_path,
                                    "row": row_idx,
                                    "col": col_idx,
                                    **item,
                                })
                if getattr(shape, "has_chart", False):
                    chart = shape.chart
                    if chart.has_title:
                        for item in extract_text_frame(chart.chart_title.text_frame):
                            deck["items"].append({
                                "slide": slide_idx,
                                "kind": "chart_title",
                                "shape_path": shape_path,
                                **item,
                            })
            # Raw XML text is retained as an audit list for text in unsupported objects.
            raw_text = [node.text or "" for node in slide.element.xpath(".//a:t")]
            deck["items"].append({"slide": slide_idx, "kind": "raw_audit", "texts": raw_text})
        result.append(deck)
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    summary = [
        {
            "file": d["file"],
            "slides": d["slides"],
            "editable_text_runs": sum(1 for i in d["items"] if i["kind"] != "raw_audit"),
            "raw_text_nodes": sum(len(i["texts"]) for i in d["items"] if i["kind"] == "raw_audit"),
        }
        for d in result
    ]
    print(json.dumps(summary, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: extract_ppt_text.py ROOT OUTPUT_JSON")
    main(Path(sys.argv[1]), Path(sys.argv[2]))
