from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


def main(render_root: Path, output_dir: Path):
    output_dir.mkdir(parents=True, exist_ok=True)
    font = ImageFont.load_default()
    thumb_w, thumb_h = 320, 180
    label_h = 22
    columns = 4
    for deck_dir in sorted([p for p in render_root.iterdir() if p.is_dir()], key=lambda p: p.name.casefold()):
        slides = sorted(deck_dir.glob("slide-*.png"))
        if not slides:
            continue
        rows = (len(slides) + columns - 1) // columns
        sheet = Image.new("RGB", (columns * thumb_w, rows * (thumb_h + label_h)), "white")
        draw = ImageDraw.Draw(sheet)
        for idx, path in enumerate(slides):
            with Image.open(path) as image:
                image = image.convert("RGB")
                image.thumbnail((thumb_w, thumb_h))
                x = (idx % columns) * thumb_w + (thumb_w - image.width) // 2
                y = (idx // columns) * (thumb_h + label_h)
                sheet.paste(image, (x, y))
            draw.text((idx % columns * thumb_w + 6, y + thumb_h + 3), path.stem, fill="black", font=font)
        output_path = output_dir / f"{deck_dir.name}.jpg"
        sheet.save(output_path, quality=88, optimize=True)
        print(output_path)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: make_contact_sheets.py RENDER_ROOT OUTPUT_DIR")
    main(Path(sys.argv[1]), Path(sys.argv[2]))
