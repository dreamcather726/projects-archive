from __future__ import annotations

import argparse
from pathlib import Path

from pptx import Presentation
from pptx.oxml.ns import qn


def keep_only_first_slide(prs: Presentation) -> None:
    slide_ids = prs.slides._sldIdLst
    for slide_id in list(slide_ids)[1:]:
        prs.part.drop_rel(slide_id.rId)
        slide_ids.remove(slide_id)


def remove_cover_text(slide) -> None:
    tree = slide.shapes._spTree
    for shape in list(slide.shapes):
        if getattr(shape, "has_text_frame", False) and shape.text.strip():
            tree.remove(shape._element)
    for tag in ("p:timing", "p:transition"):
        node = slide._element.find(qn(tag))
        if node is not None:
            slide._element.remove(node)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("sources", nargs="+", type=Path)
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)

    for source in args.sources:
        prs = Presentation(source)
        remove_cover_text(prs.slides[0])
        keep_only_first_slide(prs)
        output = args.output_dir / f"{source.stem}_clean_cover.pptx"
        prs.save(output)
        print(output)


if __name__ == "__main__":
    main()
