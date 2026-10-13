from __future__ import annotations

import argparse
from pathlib import Path

from pptx import Presentation
from pptx.dml.color import RGBColor
from pptx.enum.shapes import MSO_SHAPE
from pptx.enum.text import MSO_ANCHOR, PP_ALIGN
from pptx.oxml.ns import qn
from pptx.util import Inches, Pt


SLIDE_W = 13.333333
SLIDE_H = 7.5
FONT = "Times New Roman"


def remove_all_shapes(slide) -> None:
    sp_tree = slide.shapes._spTree
    for shape in list(slide.shapes):
        sp_tree.remove(shape._element)


def set_shape_name(shape, name: str) -> None:
    shape._element.nvSpPr.cNvPr.set("name", name)


def add_panel(slide, name, x, y, w, h, color, radius=False):
    shape_type = MSO_SHAPE.ROUNDED_RECTANGLE if radius else MSO_SHAPE.RECTANGLE
    shape = slide.shapes.add_shape(
        shape_type,
        Inches(x),
        Inches(y),
        Inches(w),
        Inches(h),
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


def add_text(
    slide,
    name,
    text,
    x,
    y,
    w,
    h,
    size,
    color,
    bold=False,
    align=PP_ALIGN.LEFT,
):
    box = slide.shapes.add_textbox(Inches(x), Inches(y), Inches(w), Inches(h))
    set_shape_name(box, name)
    frame = box.text_frame
    frame.clear()
    frame.margin_left = 0
    frame.margin_right = 0
    frame.margin_top = 0
    frame.margin_bottom = 0
    frame.vertical_anchor = MSO_ANCHOR.MIDDLE
    frame.word_wrap = False
    paragraph = frame.paragraphs[0]
    paragraph.alignment = align
    run = paragraph.add_run()
    run.text = text
    run.font.name = FONT
    run.font.size = Pt(size)
    run.font.bold = bold
    run.font.color.rgb = RGBColor(*color)
    return box


def rebuild_cover(slide, background: Path) -> None:
    remove_all_shapes(slide)
    for tag in ("p:timing", "p:transition"):
        node = slide._element.find(qn(tag))
        if node is not None:
            slide._element.remove(node)

    picture = slide.shapes.add_picture(
        str(background),
        0,
        0,
        width=Inches(SLIDE_W),
        height=Inches(SLIDE_H),
    )
    picture._element.nvPicPr.cNvPr.set("name", "Original Camera Background")

    dark = (5, 10, 14)
    white = (255, 255, 255)
    light_gray = (222, 226, 230)
    amber = (255, 187, 47)
    navy = (0, 48, 112)

    add_panel(slide, "Main Title Mask", 0.48, 1.28, 6.18, 1.18, dark)
    add_text(
        slide,
        "Editable Main Title",
        "Webcam-Based Remote Control",
        0.62,
        1.38,
        5.92,
        0.92,
        31,
        white,
        bold=True,
    )

    add_panel(slide, "Lesson Badge", 0.60, 2.59, 3.55, 0.41, amber, radius=True)
    add_text(
        slide,
        "Editable Lesson Badge Text",
        "DEXTEROUS HAND CAMP · LESSON 10",
        0.70,
        2.60,
        3.35,
        0.38,
        13,
        navy,
        bold=True,
        align=PP_ALIGN.CENTER,
    )

    add_panel(slide, "Subtitle Mask", 0.48, 3.80, 5.32, 0.50, dark)
    add_text(
        slide,
        "Editable Subtitle",
        "EFFORTLESS GESTURE-BASED REMOTE CONTROL",
        0.62,
        3.83,
        5.05,
        0.42,
        15,
        light_gray,
    )

    add_panel(slide, "Instructor Mask", 0.48, 4.64, 3.62, 1.04, dark)
    add_text(
        slide,
        "Editable Instructor",
        "Instructor: Yue Guowen",
        0.68,
        4.70,
        3.28,
        0.40,
        21,
        white,
        bold=True,
    )
    add_text(
        slide,
        "Editable Assistant",
        "Assistant: Hu Yuantao",
        0.68,
        5.22,
        3.28,
        0.40,
        21,
        white,
        bold=True,
    )

    add_panel(slide, "Livestream Badge", 0.50, 6.10, 6.12, 0.68, white, radius=True)
    add_text(
        slide,
        "Editable Livestream Text",
        "XIAOETONG INTERNAL LIVE · TUE 19:30",
        0.70,
        6.16,
        5.72,
        0.54,
        17,
        (0, 0, 0),
        bold=True,
        align=PP_ALIGN.CENTER,
    )


def build_single(background: Path, output: Path) -> None:
    prs = Presentation()
    prs.slide_width = Inches(SLIDE_W)
    prs.slide_height = Inches(SLIDE_H)
    slide = prs.slides.add_slide(prs.slide_layouts[6])
    rebuild_cover(slide, background)
    output.parent.mkdir(parents=True, exist_ok=True)
    prs.save(output)


def build_full(source: Path, background: Path, output: Path) -> None:
    prs = Presentation(source)
    rebuild_cover(prs.slides[0], background)
    output.parent.mkdir(parents=True, exist_ok=True)
    prs.save(output)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("background", type=Path)
    parser.add_argument("full_deck_source", type=Path)
    parser.add_argument("output_dir", type=Path)
    args = parser.parse_args()
    build_single(args.background, args.output_dir / "DAY10_Editable_English_Cover.pptx")
    build_full(
        args.full_deck_source,
        args.background,
        args.output_dir / "DAY10_English_FullDeck_EditableCover.pptx",
    )
    print(args.output_dir / "DAY10_Editable_English_Cover.pptx")
    print(args.output_dir / "DAY10_English_FullDeck_EditableCover.pptx")


if __name__ == "__main__":
    main()
