from __future__ import annotations

import argparse
from pathlib import Path

from pptx import Presentation
from pptx.dml.color import RGBColor
from pptx.enum.shapes import MSO_SHAPE
from pptx.enum.text import MSO_ANCHOR, PP_ALIGN
from pptx.oxml.ns import qn
from pptx.util import Inches, Pt


FONT = "Times New Roman"
SLIDE_W = 13.333333
SLIDE_H = 7.5


def set_shape_name(shape, name: str) -> None:
    shape._element.nvSpPr.cNvPr.set("name", name)


def add_panel(slide, name, x, y, w, h, color):
    shape = slide.shapes.add_shape(
        MSO_SHAPE.RECTANGLE, Inches(x), Inches(y), Inches(w), Inches(h)
    )
    shape.fill.solid()
    shape.fill.fore_color.rgb = RGBColor(*color)
    shape.line.fill.background()
    style = shape._element.find(qn("p:style"))
    if style is not None:
        effect_ref = style.find(qn("a:effectRef"))
        if effect_ref is not None:
            effect_ref.set("idx", "0")
    set_shape_name(shape, name)
    return shape


def add_text(slide, name, text, x, y, w, h, size, color, bold=False, align=PP_ALIGN.LEFT):
    box = slide.shapes.add_textbox(Inches(x), Inches(y), Inches(w), Inches(h))
    set_shape_name(box, name)
    frame = box.text_frame
    frame.clear()
    frame.margin_left = 0
    frame.margin_right = 0
    frame.margin_top = 0
    frame.margin_bottom = 0
    frame.word_wrap = True
    frame.vertical_anchor = MSO_ANCHOR.MIDDLE
    paragraph = frame.paragraphs[0]
    paragraph.alignment = align
    run = paragraph.add_run()
    run.text = text
    run.font.name = FONT
    run.font.size = Pt(size)
    run.font.bold = bold
    run.font.color.rgb = RGBColor(*color)
    return box


def format_cell(cell, text: str, size: int, bold: bool = False) -> None:
    cell.fill.solid()
    cell.fill.fore_color.rgb = RGBColor(22, 29, 37)
    cell.margin_left = Inches(0.05)
    cell.margin_right = Inches(0.05)
    cell.margin_top = Inches(0.02)
    cell.margin_bottom = Inches(0.02)
    frame = cell.text_frame
    frame.clear()
    frame.word_wrap = True
    frame.vertical_anchor = MSO_ANCHOR.MIDDLE
    paragraph = frame.paragraphs[0]
    paragraph.alignment = PP_ALIGN.CENTER
    run = paragraph.add_run()
    run.text = text
    run.font.name = FONT
    run.font.size = Pt(size)
    run.font.bold = bold
    run.font.color.rgb = RGBColor(255, 255, 255)


def add_comparison_slide(prs: Presentation, background: Path) -> None:
    slide = prs.slides.add_slide(prs.slide_layouts[6])
    picture = slide.shapes.add_picture(
        str(background), 0, 0, width=Inches(SLIDE_W), height=Inches(SLIDE_H)
    )
    picture._element.nvPicPr.cNvPr.set("name", "Original Comparison Visual")

    dark = (35, 40, 48)
    white = (255, 255, 255)

    add_panel(slide, "Title Mask", 0.38, 0.48, 5.58, 0.82, dark)
    add_text(
        slide,
        "Editable Comparison Title",
        "Structural Comparison: Striking Similarities",
        0.50,
        0.55,
        5.32,
        0.66,
        27,
        white,
        bold=True,
    )

    add_panel(slide, "Header Mask", 9.70, 0.40, 3.15, 0.38, dark)
    add_text(
        slide,
        "Editable Section Header",
        "DEXTEROUS HAND CAMP  |  HAND DISCOVERY",
        9.80,
        0.43,
        2.95,
        0.28,
        10,
        white,
        align=PP_ALIGN.CENTER,
    )

    add_panel(slide, "Comparison Table Mask", 4.83, 1.53, 3.75, 5.82, (22, 29, 37))
    rows = [
        ("Human Anatomy", "Hand Component", "Core Function"),
        ("Palm", "Palm Base", "Support &\nConnection"),
        ("Phalanges", "Finger Frame", "Structural Form"),
        ("Joints", "Pivot Shafts", "Rotation"),
        ("Joints", "Pivot Shafts", "Rotation"),
        ("Tendons", "Linkages", "Force Transfer"),
        ("Muscles", "Servo Motors", "Power"),
        ("Motor Nerves", "Arduino +\nPCA9685", "Control Signals"),
    ]
    table_shape = slide.shapes.add_table(
        len(rows), 3, Inches(4.83), Inches(1.53), Inches(3.75), Inches(5.82)
    )
    table_shape.name = "Editable Human-to-Robot Comparison Table"
    table = table_shape.table
    table.columns[0].width = Inches(1.05)
    table.columns[1].width = Inches(1.45)
    table.columns[2].width = Inches(1.25)
    for row_index, row in enumerate(rows):
        for col_index, value in enumerate(row):
            format_cell(table.cell(row_index, col_index), value, 11 if row_index else 12, row_index == 0)

    labels = [
        ("Distal joint", 8.67, 1.56, 0.92, 0.28, 11),
        ("Middle joint", 8.67, 2.18, 1.00, 0.28, 11),
        ("Proximal joint", 8.67, 2.83, 1.08, 0.28, 11),
        ("Modular fingers", 12.05, 3.12, 1.20, 0.34, 11),
        ("Thumb", 12.34, 4.42, 0.76, 0.32, 12),
        ("Palm plate", 12.22, 5.35, 0.90, 0.32, 12),
    ]
    for index, (text, x, y, w, h, size) in enumerate(labels, 1):
        if text == "Thumb":
            add_panel(slide, f"Robot Label Mask {index}", 12.14, 4.28, 1.19, 0.62, white)
        else:
            add_panel(slide, f"Robot Label Mask {index}", x - 0.05, y - 0.02, w + 0.10, h + 0.04, white)
        add_text(slide, f"Editable Robot Label {index}", text, x, y, w, h, size, (35, 35, 35), align=PP_ALIGN.CENTER)

    slide_ids = prs.slides._sldIdLst
    new_slide_id = slide_ids[-1]
    slide_ids.remove(new_slide_id)
    slide_ids.insert(27, new_slide_id)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("background", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    prs = Presentation(args.source)
    if len(prs.slides) != 31:
        raise RuntimeError(f"Expected 31 slides before restoration, found {len(prs.slides)}")
    add_comparison_slide(prs, args.background)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    prs.save(args.output)
    print(args.output)


if __name__ == "__main__":
    main()
