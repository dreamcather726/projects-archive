"""YOLO26 monocular depth from a local webcam; frames stay on this computer."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import sys
import time

import cv2
import numpy as np

ROOT = Path(__file__).resolve().parent
WINDOW = "YOLO26 Camera Depth"


def parse_args(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--camera", type=int, default=0, help="Webcam index (default: 0)")
    parser.add_argument("--backend", choices=["auto", "dshow", "msmf"], default="auto")
    parser.add_argument("--width", type=int, default=640)
    parser.add_argument("--height", type=int, default=480)
    parser.add_argument("--model", default=str(ROOT / "models" / "yolo26n-depth.pt"))
    parser.add_argument("--device", default="auto", help="auto, cpu, or CUDA index such as 0")
    parser.add_argument("--imgsz", type=int, default=768, help="768 is the pretrained resolution")
    parser.add_argument("--near", type=float, default=0.2, help="Fixed color scale near depth")
    parser.add_argument("--far", type=float, default=10.0, help="Fixed color scale far depth")
    parser.add_argument("--mirror", action="store_true", help="Mirror input and depth together")
    parser.add_argument("--frames", type=int, default=0, help="Stop after N processed frames; 0 = unlimited")
    parser.add_argument("--no-display", action="store_true", help="Run without a preview window")
    parser.add_argument("--report", type=Path, help="Write runtime summary, without camera images")
    parser.add_argument("--output", type=Path, default=ROOT / "outputs")
    parser.add_argument("--list-cameras", action="store_true", help="Probe camera indexes 0..4, then exit")
    args = parser.parse_args(argv)
    if args.width <= 0 or args.height <= 0 or args.imgsz <= 0 or args.frames < 0:
        parser.error("width, height, imgsz must be positive; frames must be >= 0")
    if args.camera < 0 or not 0 < args.near < args.far:
        parser.error("camera must be >= 0 and 0 < near < far")
    if args.no_display and args.frames == 0 and not args.list_cameras:
        parser.error("--no-display requires --frames N for a bounded run")
    return args


def open_camera(index, backend="auto", width=640, height=480):
    """Try Windows backends in order, including a real first-frame check."""
    backends = {"dshow": cv2.CAP_DSHOW, "msmf": cv2.CAP_MSMF}
    choices = (["dshow", "msmf", "default"] if sys.platform == "win32" else ["default"])
    if backend != "auto":
        choices = [backend]
    for name in choices:
        cap = cv2.VideoCapture(index, backends.get(name, cv2.CAP_ANY))
        try:
            if cap.isOpened():
                cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
                cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
                cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
                # Allow auto-exposure and device startup to settle.
                for _ in range(10):
                    ok, frame = cap.read()
                    if ok and frame is not None and frame.size:
                        return cap, frame, name
                    time.sleep(0.05)
        except BaseException:
            cap.release()
            raise
        cap.release()
    raise RuntimeError(
        f"Camera {index} could not provide a frame. Close other camera apps, enable "
        "Windows Settings > Privacy & security > Camera > desktop app access, "
        "check the lens shutter, or try --camera 1 / --backend msmf."
    )


def sample_depth(depth, x, y, radius=4):
    """Median of valid pixels in a small patch, clipped at image edges."""
    h, w = depth.shape
    if not (0 <= x < w and 0 <= y < h):
        return None
    patch = depth[max(0, y-radius):min(h, y+radius+1), max(0, x-radius):min(w, x+radius+1)]
    values = patch[np.isfinite(patch) & (patch > 0)]
    return float(np.median(values)) if values.size else None


def colorize_depth(depth, near, far):
    """Fixed depth range across frames: near=warm, far=cool; invalid=black."""
    valid = np.isfinite(depth) & (depth > 0)
    clean = np.where(valid, depth, far)
    intensity = np.uint8(255 * (1 - np.clip((clean-near)/(far-near), 0, 1)))
    colored = cv2.applyColorMap(intensity, cv2.COLORMAP_TURBO)
    colored[~valid] = 0
    return colored


def render_view(frame, depth, point, fps, device, near, far):
    h, w = frame.shape[:2]
    colored = colorize_depth(depth, near, far)
    canvas = np.hstack((frame.copy(), colored))
    x, y = point or (w//2, h//2)
    meters = sample_depth(depth, x, y)
    for offset in (0, w):
        cv2.drawMarker(canvas, (x+offset, y), (255, 255, 255), cv2.MARKER_CROSS, 20, 2)
    header = np.zeros((92, w*2, 3), dtype=np.uint8)
    reading = f"{meters:.2f} m (estimated)" if meters is not None else "invalid depth"
    for line, text in enumerate((
        f"RGB | YOLO26 depth   inference {fps:.1f} FPS   device={device}",
        f"Point ({x}, {y}): {reading} | colors: {near:g}m warm -> {far:g}m cool",
        "Click: probe | C: center | S: save | Q / Esc: quit",
    )):
        cv2.putText(header, text, (10, 23+line*27), cv2.FONT_HERSHEY_SIMPLEX, 0.52, (230, 230, 230), 1, cv2.LINE_AA)
    return np.vstack((header, canvas))


def write_image(path, image):
    # imwrite on some Windows builds cannot handle Chinese paths.
    ok, encoded = cv2.imencode(".png", image)
    if not ok:
        raise RuntimeError(f"Failed to encode {path}")
    encoded.tofile(str(path))


def save_snapshot(output, frame, depth, view, metadata):
    folder = output / datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    folder.mkdir(parents=True, exist_ok=False)
    write_image(folder / "rgb.png", frame)
    write_image(folder / "preview.png", view)
    np.save(folder / "depth_estimated_m.npy", depth.astype(np.float32))
    (folder / "metadata.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")
    print(f"Saved snapshot: {folder}", flush=True)


def run(args):
    if args.list_cameras:
        found = []
        for index in range(5):
            try:
                cap, frame, backend = open_camera(index, args.backend, args.width, args.height)
            except RuntimeError:
                continue
            cap.release()
            found.append(index)
            print(f"Camera {index}: {frame.shape[1]}x{frame.shape[0]} ({backend})")
        if not found:
            raise RuntimeError("No usable cameras found at indexes 0..4.")
        return

    import torch
    import ultralytics
    from ultralytics import YOLO

    device = ("0" if torch.cuda.is_available() else "cpu") if args.device == "auto" else args.device
    model_path = Path(args.model).expanduser()
    model_path.parent.mkdir(parents=True, exist_ok=True)
    print(f"Loading {model_path.name}; device={device}; first run may download official weights.", flush=True)
    model = YOLO(str(model_path), task="depth")
    if model.task != "depth":
        raise RuntimeError("A YOLO26 -depth model is required; detection weights do not estimate depth.")
    # Warm up before opening the webcam so downloads do not leave it occupied.
    model.predict(np.zeros((args.height, args.width, 3), np.uint8), imgsz=args.imgsz,
                  device=device, verbose=False, save=False)
    cap = None
    count, durations, center_values = 0, [], []
    point = None
    window_open = False
    started = time.perf_counter()
    try:
        cap, frame, backend = open_camera(args.camera, args.backend, args.width, args.height)
        h, w = frame.shape[:2]
        if not args.no_display:
            cv2.namedWindow(WINDOW, cv2.WINDOW_AUTOSIZE)
            window_open = True

            def on_mouse(event, x, y, flags, param):
                nonlocal point
                # Header is 92 pixels; both panels map to the same depth coordinates.
                if event == cv2.EVENT_LBUTTONDOWN and 0 <= x < 2*w and 92 <= y < h+92:
                    point = (x % w, y-92)

            cv2.setMouseCallback(WINDOW, on_mouse)
        print(f"Camera {args.camera} opened: {w}x{h}, backend={backend}. Q closes the preview.", flush=True)
        while True:
            tick = time.perf_counter()
            if args.mirror:
                frame = cv2.flip(frame, 1)
            result = model.predict(frame, imgsz=args.imgsz, device=device, verbose=False, save=False)[0]
            if result.depth is None:
                raise RuntimeError("No depth output. Install requirements.txt and use yolo26n-depth.pt.")
            depth = result.depth.data.detach().float().cpu().numpy()
            if depth.shape != frame.shape[:2]:
                raise RuntimeError(f"Depth/RGB shape mismatch: {depth.shape} / {frame.shape[:2]}")
            center = sample_depth(depth, w//2, h//2)
            if center is None and not np.any(np.isfinite(depth) & (depth > 0)):
                raise RuntimeError("Model returned no valid positive depth pixels.")
            if center is not None:
                center_values.append(center)
            inference_seconds = time.perf_counter()-tick
            key = -1
            if not args.no_display:
                view = render_view(frame, depth, point, 1/max(inference_seconds, 1e-9), device, args.near, args.far)
                cv2.imshow(WINDOW, view)
                key = cv2.waitKey(1) & 0xFF
                if key == ord("c"):
                    point = None
                elif key == ord("s"):
                    save_snapshot(args.output, frame, depth, view, {
                        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
                        "model": model_path.name, "device": device, "mirror": args.mirror,
                        "near_m": args.near, "far_m": args.far,
                        "unit": "estimated meters; not measured ground truth",
                    })
            count += 1
            durations.append(inference_seconds)
            if key in (27, ord("q")) or (args.frames and count >= args.frames):
                break
            if window_open and cv2.getWindowProperty(WINDOW, cv2.WND_PROP_VISIBLE) < 1:
                break
            ok, frame = cap.read()
            if not ok or frame is None:
                raise RuntimeError("Camera stopped delivering frames; reconnect it and restart.")
    finally:
        if cap is not None:
            cap.release()
        if window_open:
            cv2.destroyAllWindows()

    report = {
        "timestamp_utc": datetime.now(timezone.utc).isoformat(), "status": "ok",
        "model": model_path.name, "ultralytics": ultralytics.__version__, "torch": torch.__version__,
        "device": device, "cuda_available": torch.cuda.is_available(),
        "camera": args.camera, "backend": backend, "resolution": [w, h], "imgsz": args.imgsz,
        "frames": count, "elapsed_seconds": round(time.perf_counter()-started, 3),
        "mean_inference_ms": round(float(np.mean(durations))*1000, 2),
        "center_depth_median_estimated_m": float(np.median(center_values)) if center_values else None,
        "accuracy_validated": False,
    }
    print(json.dumps(report, indent=2), flush=True)
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")


def main():
    try:
        run(parse_args())
        return 0
    except KeyboardInterrupt:
        print("Stopped; camera released.")
        return 130
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
