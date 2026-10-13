from __future__ import annotations

import argparse
from dataclasses import dataclass
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


@dataclass(frozen=True)
class Profile:
    day: int
    title: str
    subtitle: str
    live_day: str | None
    title_size: int = 31
    subtitle_size: int = 14
    live_y: float = 6.12
    live_x: float = 0.50
    live_w: float = 6.12


PROFILES = [
    Profile(1, "Dexterous Hand Discovery", "FROM IMAGINATION TO REALITY  |  TOUCH THE FUTURE", None, 31, 14),
    Profile(2, "Mechanical Magic &\nSkeleton Assembly", "LEVERS, LINKAGES, AND HANDS-ON MECHANICAL ASSEMBLY", "THU", 30, 13, 6.38),
    Profile(3, "Servo Motors:\nThe Hand's Muscles", "BRING THE STATIC FRAME TO LIFE  |  MASTER THE POWER CORE", "TUE", 30, 13, 5.96, 0.65, 6.08),
    Profile(4, "Servo-to-Skeleton\nIntegration", "FROM STATIC STRUCTURE TO PRECISE MOTION", "THU", 30, 14, 6.55, 0.34, 6.08),
    Profile(5, "Planar Mechanism\nFundamentals", "UNLOCK THE PRINCIPLES BEHIND MECHANICAL MOTION", "TUE", 30, 13, 6.38),
    Profile(6, "Planar Mechanism\nDegrees of Freedom", "THE HIDDEN CODE OF MECHANISM MOTION", "THU", 29, 14),
    Profile(7, "Servo Control via\nSerial & I²C", "FROM PRINCIPLES TO CODE  |  MASTER CORE COMMUNICATION", "TUE", 30, 13),
    Profile(8, "ESP32 Coding in Practice", "PROGRAM PURPOSE, STRUCTURE, AND IMPLEMENTATION LOGIC", "THU", 31, 13),
    Profile(9, "Dexterous Hand\nController Programming", "A HANDS-ON PROGRAMMING PROJECT MADE EASY", "THU", 29, 14),
]


def belongs_to_day(name: str, day: int) -> bool:
    value = name.casefold()
    if day == 1:
        return value.startswith("day1") and not value.startswith("day11")
    if day in (2, 3, 4, 5, 6, 9):
        return value.startswith(f"day{day}")
    if day == 7:
        return "i2c" in value
    if day == 8:
        return "esp32" in value
    return False


def find_deck(root: Path, day: int) -> Path:
    matches = [
        path
        for path in root.glob("*.pptx")
        if not path.name.startswith("~$") and belongs_to_day(path.name, day)
    ]
    if len(matches) != 1:
        raise RuntimeError(f"Expected one source deck for day {day}, found {matches}")
    return matches[0]


def find_background(root: Path, day: int) -> Path:
    matches = [
        path / "slide-001.png"
        for path in root.iterdir()
        if path.is_dir() and belongs_to_day(path.name, day)
    ]
    if len(matches) != 1 or not matches[0].exists():
        raise RuntimeError(f"Expected one background for day {day}, found {matches}")
    return matches[0]


def remove_all_shapes(slide) -> None:
    tree = slide.shapes._spTree
    for shape in list(slide.shapes):
        tree.remove(shape._element)


def clear_slide_effects(slide) -> None:
    for tag in ("p:timing", "p:transition"):
        node = slide._element.find(qn(tag))
        if node is not None:
            slide._element.remove(node)


def set_shape_name(shape, name: str) -> None:
    shape._element.nvSpPr.cNvPr.set("name", name)


def add_panel(slide, name, x, y, w, h, color, rounded=False):
    kind = MSO_SHAPE.ROUNDED_RECTANGLE if rounded else MSO_SHAPE.RECTANGLE
    shape = slide.shapes.add_shape(kind, Inches(x), Inches(y), Inches(w), Inches(h))
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
    frame.vertical_anchor = MSO_ANCHOR.MIDDLE
    frame.word_wrap = True
    paragraph = frame.paragraphs[0]
    paragraph.alignment = align
    for index, line in enumerate(text.split("\n")):
        target = paragraph if index == 0 else frame.add_paragraph()
        target.alignment = align
        target.space_before = Pt(0)
        target.space_after = Pt(0)
        run = target.add_run()
        run.text = line
        run.font.name = FONT
        run.font.size = Pt(size)
        run.font.bold = bold
        run.font.color.rgb = RGBColor(*color)
    return box


def rebuild_cover(slide, background: Path, profile: Profile) -> None:
    remove_all_shapes(slide)
    clear_slide_effects(slide)
    picture = slide.shapes.add_picture(
        str(background), 0, 0, width=Inches(SLIDE_W), height=Inches(SLIDE_H)
    )
    picture._element.nvPicPr.cNvPr.set("name", f"Day {profile.day} Clean Background")

    dark = (5, 10, 14)
    white = (255, 255, 255)
    light_gray = (224, 228, 232)
    amber = (255, 187, 47)
    navy = (0, 48, 112)

    add_panel(slide, "Main Title Panel", 0.48, 1.18, 6.25, 1.35, dark)
    add_text(
        slide,
        "Editable Main Title",
        profile.title,
        0.62,
        1.26,
        5.95,
        1.15,
        profile.title_size,
        white,
        bold=True,
    )

    add_panel(slide, "Lesson Badge", 0.60, 2.68, 3.58, 0.43, amber, rounded=True)
    add_text(
        slide,
        "Editable Lesson Badge Text",
        f"DEXTEROUS HAND CAMP · LESSON {profile.day}",
        0.70,
        2.70,
        3.38,
        0.38,
        13,
        navy,
        bold=True,
        align=PP_ALIGN.CENTER,
    )

    add_panel(slide, "Subtitle Panel", 0.48, 3.40, 5.55, 0.50, dark)
    add_text(
        slide,
        "Editable Subtitle",
        profile.subtitle,
        0.62,
        3.43,
        5.27,
        0.42,
        profile.subtitle_size,
        light_gray,
    )

    add_panel(slide, "Instructor Panel", 0.48, 4.64, 3.62, 1.04, dark)
    add_text(slide, "Editable Instructor", "Instructor: Yue Guowen", 0.68, 4.70, 3.28, 0.40, 21, white, True)
    add_text(slide, "Editable Assistant", "Assistant: Hu Yuantao", 0.68, 5.22, 3.28, 0.40, 21, white, True)

    if profile.live_day:
        add_panel(
            slide,
            "Livestream Badge",
            profile.live_x,
            profile.live_y,
            profile.live_w,
            0.66,
            white,
            rounded=True,
        )
        add_text(
            slide,
            "Editable Livestream Text",
            f"XIAOETONG INTERNAL LIVE · {profile.live_day} 19:30",
            profile.live_x + 0.20,
            profile.live_y + 0.05,
            profile.live_w - 0.40,
            0.54,
            17,
            (0, 0, 0),
            True,
            align=PP_ALIGN.CENTER,
        )


def build_single(background: Path, profile: Profile, output: Path) -> None:
    prs = Presentation()
    prs.slide_width = Inches(SLIDE_W)
    prs.slide_height = Inches(SLIDE_H)
    slide = prs.slides.add_slide(prs.slide_layouts[6])
    rebuild_cover(slide, background, profile)
    prs.save(output)


def build_full(source: Path, background: Path, profile: Profile, output: Path) -> None:
    prs = Presentation(source)
    rebuild_cover(prs.slides[0], background, profile)
    prs.save(output)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("background_root", type=Path)
    parser.add_argument("source_root", type=Path)
    parser.add_argument("output_dir", type=Path)
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)

    for profile in PROFILES:
        source = find_deck(args.source_root, profile.day)
        background = find_background(args.background_root, profile.day)
        single = args.output_dir / f"DAY{profile.day}_Editable_English_Cover.pptx"
        full = args.output_dir / f"DAY{profile.day}_English_FullDeck_EditableCover.pptx"
        build_single(background, profile, single)
        print(single)
        build_full(source, background, profile, full)
        print(full)


if __name__ == "__main__":
    main()
