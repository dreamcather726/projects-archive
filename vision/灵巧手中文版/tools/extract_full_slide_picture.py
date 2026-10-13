from __future__ import annotations

import argparse
from pathlib import Path

from pptx import Presentation
from pptx.enum.shapes import MSO_SHAPE_TYPE


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    prs = Presentation(args.source)
    slide = prs.slides[0]
    candidates = [
        shape
        for shape in slide.shapes
        if shape.shape_type == MSO_SHAPE_TYPE.PICTURE
        and shape.left == 0
        and shape.top == 0
        and shape.width == prs.slide_width
        and shape.height == prs.slide_height
    ]
    if not candidates:
        raise RuntimeError("No full-slide picture found on slide 1")

    picture = candidates[-1]
    suffix = "." + picture.image.ext
    output = args.output.with_suffix(suffix)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(picture.image.blob)
    print(output)


if __name__ == "__main__":
    main()
