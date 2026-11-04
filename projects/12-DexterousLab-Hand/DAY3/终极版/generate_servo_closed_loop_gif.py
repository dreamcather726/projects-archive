import math
from dataclasses import dataclass
from pathlib import Path
from typing import Tuple

from PIL import Image, ImageDraw, ImageFont


@dataclass
class Palette:
    bg: Tuple[int, int, int] = (11, 16, 32)
    panel: Tuple[int, int, int] = (18, 26, 51)
    line: Tuple[int, int, int] = (60, 74, 122)
    text: Tuple[int, int, int] = (232, 236, 255)
    muted: Tuple[int, int, int] = (170, 179, 221)
    hi: Tuple[int, int, int] = (122, 162, 255)
    ok: Tuple[int, int, int] = (66, 211, 146)
    warn: Tuple[int, int, int] = (255, 204, 102)
    bad: Tuple[int, int, int] = (255, 107, 107)


def clamp(v: float, a: float, b: float) -> float:
    return max(a, min(b, v))


def load_font(size: int) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    candidates = [
        r"C:\Windows\Fonts\msyh.ttc",
        r"C:\Windows\Fonts\msyh.ttf",
        r"C:\Windows\Fonts\simhei.ttf",
        r"C:\Windows\Fonts\simsun.ttc",
    ]
    for path in candidates:
        try:
            return ImageFont.truetype(path, size)
        except Exception:
            pass
    return ImageFont.load_default()


def rounded_rect(draw: ImageDraw.ImageDraw, xy, r: int, fill, outline, width: int = 2):
    draw.rounded_rectangle(xy, radius=r, fill=fill, outline=outline, width=width)


def draw_arrow(
    draw: ImageDraw.ImageDraw,
    p0: Tuple[float, float],
    p1: Tuple[float, float],
    color,
    width: int = 4,
    head_len: int = 12,
    head_w: int = 10,
):
    draw.line([p0, p1], fill=color, width=width)
    ang = math.atan2(p1[1] - p0[1], p1[0] - p0[0])
    back = (p1[0] - head_len * math.cos(ang), p1[1] - head_len * math.sin(ang))
    left = (
        back[0] + head_w * math.cos(ang + math.pi / 2),
        back[1] + head_w * math.sin(ang + math.pi / 2),
    )
    right = (
        back[0] + head_w * math.cos(ang - math.pi / 2),
        back[1] + head_w * math.sin(ang - math.pi / 2),
    )
    draw.polygon([p1, left, right], fill=color)


def gear_polygon(cx: float, cy: float, r: float, teeth: int, angle_deg: float) -> list[Tuple[float, float]]:
    pts: list[Tuple[float, float]] = []
    angle0 = math.radians(angle_deg)
    for i in range(teeth * 2):
        ang = angle0 + i * math.pi / teeth
        rr = r if i % 2 == 0 else r * 0.82
        pts.append((cx + rr * math.cos(ang), cy + rr * math.sin(ang)))
    return pts


def draw_gear(
    draw: ImageDraw.ImageDraw,
    cx: float,
    cy: float,
    r: float,
    teeth: int,
    angle_deg: float,
    fill,
    outline,
    hub_fill,
):
    pts = gear_polygon(cx, cy, r, teeth, angle_deg)
    draw.polygon(pts, fill=fill, outline=outline)
    draw.ellipse([cx - r * 0.28, cy - r * 0.28, cx + r * 0.28, cy + r * 0.28], fill=hub_fill, outline=outline, width=2)
    draw.ellipse([cx - 3, cy - 3, cx + 3, cy + 3], fill=outline)


def draw_timeline(draw: ImageDraw.ImageDraw, pal: Palette, x0: int, y0: int, w: int, h: int, cursor_t: float, font_small):
    rounded_rect(draw, (x0, y0, x0 + w, y0 + h), r=12, fill=(10, 14, 30), outline=tuple(int(c * 0.9) for c in pal.line), width=2)
    draw.text((x0 + 16, y0 + 10), "20ms 控制周期（每次：读信号→比较→切换电流→反馈）", fill=pal.text, font=font_small)
    base_y = y0 + h - 12
    inner_x0 = x0 + 220
    inner_x1 = x0 + w - 18
    draw.line((inner_x0, base_y, inner_x1, base_y), fill=pal.muted, width=3)
    for ms, label in [(0, "0ms"), (10, "10ms"), (20, "20ms")]:
        tx = inner_x0 + (inner_x1 - inner_x0) * (ms / 20)
        draw.line((tx, base_y - 10, tx, base_y + 2), fill=pal.muted, width=2)
        draw.text((tx - 10, base_y - 30), label, fill=pal.muted, font=font_small)
    cursor_x = inner_x0 + (inner_x1 - inner_x0) * clamp(cursor_t, 0.0, 1.0)
    draw_arrow(draw, (cursor_x, base_y - 24), (cursor_x, base_y + 2), pal.hi, width=3, head_len=10, head_w=6)


def draw_pwm(draw: ImageDraw.ImageDraw, pal: Palette, x: int, y: int, target_deg: float, blink: float, font_small):
    draw.rectangle((x, y, x + 200, y + 8), fill=(122, 162, 255, 50))
    pulse_w = 40 + (target_deg / 180.0) * 120
    alpha = int(120 + 135 * blink)
    draw.rectangle((x, y - 8, x + pulse_w, y + 8), fill=(pal.hi[0], pal.hi[1], pal.hi[2], alpha))
    draw.text((x, y + 14), "PWM（周期固定，脉宽=目标角度）", fill=pal.muted, font=font_small)


def draw_servo_scene(
    frame_idx: int,
    total_frames: int,
    target_deg: float,
    current_deg: float,
    mode: str,
    motor_cmd: float,
    gear_ang: float,
    disturbed: bool,
) -> Image.Image:
    pal = Palette()
    W, H = 1100, 560
    img = Image.new("RGBA", (W, H), pal.bg)
    draw = ImageDraw.Draw(img, "RGBA")

    font_title = load_font(18)
    font_label = load_font(14)
    font_small = load_font(12)

    def box(x0, y0, x1, y1, hi: bool = False):
        outline = pal.hi if hi else pal.line
        fill = (10, 14, 30, 180) if hi else (10, 14, 30, 140)
        rounded_rect(draw, (x0, y0, x1, y1), r=12, fill=fill, outline=outline, width=2)

    status = "闭环锁位" if mode == "L" else ("正向电流驱动" if mode == "F" else "反向电流驱动")
    e = target_deg - current_deg
    sign = "+" if e > 0 else ("" if e < 0 else "")
    draw.text((20, 18), f"状态：{status}", fill=pal.text, font=font_title)
    draw.text((20, 44), f"目标角度={int(round(target_deg))}°，当前角度={int(round(current_deg))}°，误差 e=目标-当前={sign}{int(round(e))}°", fill=pal.muted, font=font_label)

    controller_hi = frame_idx < int(total_frames * 0.18)
    brain_hi = int(total_frames * 0.10) <= frame_idx < int(total_frames * 0.45)
    act_hi = frame_idx >= int(total_frames * 0.22)

    box(20, 90, 260, 220, hi=controller_hi)
    draw.text((40, 115), "控制器（Arduino等）", fill=pal.text, font=font_label)
    draw.text((40, 140), "步骤1：发送 PWM 脉冲 → 目标角度", fill=pal.muted, font=font_small)
    blink = 0.5 + 0.5 * math.sin(frame_idx * 0.35)
    draw_pwm(draw, pal, 40, 190, target_deg, blink, font_small)

    box(320, 90, 640, 310, hi=brain_hi)
    draw.text((340, 115), "舵机控制电路（“大脑”）", fill=pal.text, font=font_label)
    draw.text((340, 140), "步骤2：解码 PWM → 目标角度", fill=pal.muted, font=font_small)
    draw.text((340, 160), "读取电位器 → 当前角度", fill=pal.muted, font=font_small)
    draw.text((340, 180), "步骤3：比较误差 e → 决定电流方向", fill=pal.muted, font=font_small)
    rounded_rect(draw, (340, 220, 620, 262), r=10, fill=(255, 255, 255, 16), outline=tuple(int(c * 0.9) for c in pal.line), width=2)
    draw.text((354, 234), f"目标={int(round(target_deg))}°  当前={int(round(current_deg))}°  e={sign}{int(round(e))}°", fill=pal.text, font=font_label)

    box(700, 90, 1080, 455, hi=act_hi)
    draw.text((720, 115), "执行机构（更像机械结构）", fill=pal.text, font=font_label)
    draw.text((720, 140), "H桥驱动 → 电机 → 齿轮箱 → 输出轴", fill=pal.muted, font=font_small)
    draw.text((720, 160), "外力扰动会推偏输出轴，但闭环会拉回目标角度", fill=pal.muted, font=font_small)

    box(720, 195, 880, 275, hi=brain_hi)
    box(900, 195, 1060, 275, hi=act_hi and mode != "L")
    draw.text((740, 215), "H桥/驱动", fill=pal.text, font=font_label)
    draw.text((920, 215), "直流电机", fill=pal.text, font=font_label)

    if mode == "F":
        m_text = "电流：正向"
        dir_text = "转向：正转"
        cur_color = pal.ok
    elif mode == "R":
        m_text = "电流：反向"
        dir_text = "转向：反转"
        cur_color = pal.bad
    else:
        m_text = "电流：断开/极小保持"
        dir_text = "转向：停止（锁位）"
        cur_color = pal.warn

    draw.text((740, 242), m_text, fill=pal.muted, font=font_small)
    draw.text((920, 242), dir_text, fill=pal.muted, font=font_small)

    draw_arrow(draw, (640, 235), (720, 235), pal.hi if brain_hi else pal.muted, width=3, head_len=12, head_w=7)
    draw.text((650, 214), "驱动指令", fill=pal.muted, font=font_small)

    draw_arrow(draw, (880, 235), (900, 235), cur_color, width=4, head_len=12, head_w=8)

    gear1_c = (880 + 900) / 2 - 30, 345
    gear2_c = gear1_c[0] + 70, gear1_c[1] + 10
    gear_fill = (255, 255, 255, 18)
    gear_outline = tuple(int(c * 0.95) for c in pal.line)
    hub_fill = (255, 255, 255, 30)

    speed = motor_cmd * 12.0
    if mode == "R":
        a1 = gear_ang
        a2 = -gear_ang * 1.15
    elif mode == "F":
        a1 = -gear_ang
        a2 = gear_ang * 1.15
    else:
        a1 = -gear_ang * 0.08
        a2 = gear_ang * 0.08

    draw_gear(draw, gear1_c[0], gear1_c[1], 34, 12, a1, gear_fill, gear_outline, hub_fill)
    draw_gear(draw, gear2_c[0], gear2_c[1], 44, 14, a2, gear_fill, gear_outline, hub_fill)
    draw.text((gear1_c[0] - 58, gear1_c[1] + 54), "齿轮箱", fill=pal.muted, font=font_small)

    sx, sy = 890, 350
    draw.ellipse((sx - 86, sy - 86, sx + 86, sy + 86), fill=(255, 255, 255, 10), outline=tuple(int(c * 0.95) for c in pal.line), width=2)
    draw.ellipse((sx - 4, sy - 4, sx + 4, sy + 4), fill=pal.text)

    def needle(angle_deg: float, length: float, color, width: int):
        ang = math.radians(angle_deg - 90)
        x2 = sx + length * math.cos(ang)
        y2 = sy + length * math.sin(ang)
        draw.line((sx, sy, x2, y2), fill=color, width=width)
        draw.ellipse((x2 - 4, y2 - 4, x2 + 4, y2 + 4), fill=color)

    needle(target_deg, 78, pal.hi, 4)
    needle(current_deg, 70, pal.ok, 6)
    draw.text((sx - 18, sy - 108), "目标", fill=pal.muted, font=font_small)
    draw.text((sx - 18, sy - 88), "当前", fill=pal.muted, font=font_small)

    draw.text((sx - 70, sy + 104), "输出轴角度", fill=pal.muted, font=font_small)
    draw.text((sx - 68, sy + 126), f"{int(round(current_deg))}°", fill=pal.text, font=font_label)

    if disturbed:
        draw_arrow(draw, (sx + 120, sy - 40), (sx + 70, sy - 10), pal.bad, width=5, head_len=14, head_w=10)
        rounded_rect(draw, (sx + 86, sy - 78, sx + 204, sy - 52), r=10, fill=(255, 107, 107, 28), outline=pal.bad, width=2)
        draw.text((sx + 96, sy - 74), "外力扰动", fill=pal.bad, font=font_small)

    y_mech = 300
    draw_arrow(draw, (980, 275), (980, y_mech), pal.muted, width=2, head_len=10, head_w=6)
    draw_arrow(draw, (980, y_mech), (920, y_mech + 24), pal.muted, width=2, head_len=10, head_w=6)
    draw_arrow(draw, (920, y_mech + 24), (860, y_mech + 44), pal.muted, width=2, head_len=10, head_w=6)
    draw_arrow(draw, (860, y_mech + 44), (840, y_mech + 64), pal.muted, width=2, head_len=10, head_w=6)

    draw_arrow(draw, (820, 430), (480, 300), pal.hi if act_hi else pal.muted, width=3, head_len=12, head_w=7)
    draw.text((560, 490), "电位器反馈（当前角度）", fill=pal.muted, font=font_small)

    cursor_t = (frame_idx % 20) / 20.0
    draw_timeline(draw, pal, 20, 500, 1060, 46, cursor_t, font_small)

    return img


def main():
    out_dir = Path(__file__).resolve().parent
    out_gif = out_dir / "舵机闭环控制_机械版.gif"

    target_deg = 120.0
    current_deg = 30.0
    gear_ang = 0.0

    total_frames = 140
    frames: list[Image.Image] = []

    for i in range(total_frames):
        disturbed = i == 70
        if disturbed:
            current_deg = clamp(current_deg + 35.0, 0.0, 180.0)

        e = target_deg - current_deg
        if abs(e) <= 1.5:
            mode = "L"
            motor_cmd = 0.0
        else:
            mode = "F" if e > 0 else "R"
            motor_cmd = clamp(abs(e) / 60.0, 0.18, 1.0)

        step = motor_cmd * 4.5
        if mode == "F":
            current_deg = clamp(current_deg + step, 0.0, 180.0)
        elif mode == "R":
            current_deg = clamp(current_deg - step, 0.0, 180.0)

        gear_ang += motor_cmd * 12.0

        img = draw_servo_scene(
            frame_idx=i,
            total_frames=total_frames,
            target_deg=target_deg,
            current_deg=current_deg,
            mode=mode,
            motor_cmd=motor_cmd,
            gear_ang=gear_ang,
            disturbed=disturbed,
        )
        frames.append(img.convert("P", palette=Image.Palette.ADAPTIVE, colors=255))

    frames[0].save(
        out_gif,
        save_all=True,
        append_images=frames[1:],
        duration=60,
        loop=0,
        optimize=True,
        disposal=2,
    )


if __name__ == "__main__":
    main()

