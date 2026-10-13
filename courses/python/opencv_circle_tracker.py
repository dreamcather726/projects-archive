"""Track a concentric-circle target with a computer camera.

Run:
    python opencv_circle_tracker.py

Press Q or Esc to quit. Use the on-screen trackbars to tune detection.
"""

from __future__ import annotations

import argparse
import time

import cv2
import numpy as np


WINDOW_NAME = "Concentric Circle Tracker"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--camera", type=int, default=0, help="camera index")
    parser.add_argument("--width", type=int, default=640)
    parser.add_argument("--height", type=int, default=480)
    parser.add_argument(
        "--min-rings",
        type=int,
        default=3,
        help="minimum number of concentric rings required",
    )
    return parser.parse_args()


def nothing(_value: int) -> None:
    """OpenCV trackbar callback."""


def radial_ring_peaks(
    edges: np.ndarray,
    center_x: int,
    center_y: int,
    min_radius: int,
    max_radius: int,
) -> tuple[list[int], float]:
    """Count circular edge bands around a possible target center."""
    height, width = edges.shape
    available_radius = min(
        center_x,
        center_y,
        width - center_x - 1,
        height - center_y - 1,
        max_radius,
    )
    if available_radius <= min_radius:
        return [], 0.0

    radii = np.arange(max(min_radius, 3), available_radius + 1)
    angles = np.linspace(0.0, 2.0 * np.pi, 240, endpoint=False)
    sample_x = np.rint(
        center_x + radii[:, None] * np.cos(angles)[None, :]
    ).astype(np.int32)
    sample_y = np.rint(
        center_y + radii[:, None] * np.sin(angles)[None, :]
    ).astype(np.int32)

    profile = (edges[sample_y, sample_x] > 0).mean(axis=1)
    profile = np.convolve(profile, np.ones(3) / 3.0, mode="same")

    raw_peaks = []
    for index in range(1, len(profile) - 1):
        if (
            profile[index] >= 0.08
            and profile[index] >= profile[index - 1]
            and profile[index] > profile[index + 1]
        ):
            raw_peaks.append((int(radii[index]), float(profile[index])))

    # A thick printed line normally produces two nearby Canny edges. Merge them.
    merged_peaks: list[tuple[int, float]] = []
    for radius, strength in raw_peaks:
        if merged_peaks and radius - merged_peaks[-1][0] <= 7:
            if strength > merged_peaks[-1][1]:
                merged_peaks[-1] = (radius, strength)
        else:
            merged_peaks.append((radius, strength))

    return [radius for radius, _ in merged_peaks], sum(
        strength for _, strength in merged_peaks
    )


def find_target(
    gray: np.ndarray,
    hough_threshold: int,
    min_radius: int,
    max_radius: int,
    min_rings: int,
) -> tuple[tuple[int, int, int, int] | None, list[tuple[int, int, int]]]:
    """Detect candidate centers, then verify their concentric edge bands."""
    blurred = cv2.GaussianBlur(gray, (9, 9), 1.8)
    edges = cv2.Canny(blurred, 60, 140)
    detected = cv2.HoughCircles(
        blurred,
        cv2.HOUGH_GRADIENT,
        dp=1.2,
        minDist=max(24, min_radius // 2),
        param1=120,
        param2=max(hough_threshold, 1),
        minRadius=max(min_radius, 1),
        maxRadius=max(max_radius, min_radius + 1),
    )

    if detected is None:
        return None, []

    # Hough results are ordered by accumulator strength. Evaluating the first
    # candidates keeps the live camera loop responsive in visually busy scenes.
    circles = np.round(detected[0][:30]).astype(int)
    visible_circles = [tuple(int(value) for value in circle) for circle in circles]

    best_target = None
    best_score = (-1, -1.0, -1)
    for x, y, candidate_radius in circles:
        search_radius = min(max_radius, int(candidate_radius * 1.35) + 10)
        rings, ring_strength = radial_ring_peaks(
            edges,
            int(x),
            int(y),
            max(5, min_radius // 3),
            search_radius,
        )
        if len(rings) < min_rings:
            continue

        outer_radius = rings[-1]
        target = (int(x), int(y), outer_radius, len(rings))
        score = (len(rings), ring_strength, outer_radius)
        if score > best_score:
            best_score = score
            best_target = target

    return best_target, visible_circles


def main() -> None:
    args = parse_args()

    capture = cv2.VideoCapture(args.camera, cv2.CAP_DSHOW)
    capture.set(cv2.CAP_PROP_FRAME_WIDTH, args.width)
    capture.set(cv2.CAP_PROP_FRAME_HEIGHT, args.height)

    if not capture.isOpened():
        raise SystemExit(
            f"Cannot open camera {args.camera}. Try --camera 1 or close other camera apps."
        )

    cv2.namedWindow(WINDOW_NAME, cv2.WINDOW_NORMAL)
    cv2.createTrackbar("Detection threshold", WINDOW_NAME, 30, 80, nothing)
    cv2.createTrackbar("Minimum radius", WINDOW_NAME, 20, 300, nothing)
    cv2.createTrackbar("Maximum radius", WINDOW_NAME, 220, 400, nothing)

    smoothed_x = None
    smoothed_y = None
    smoothed_radius = None
    smoothing = 0.25
    missing_frames = 0
    previous_time = time.perf_counter()

    try:
        while True:
            ok, frame = capture.read()
            if not ok:
                print("Failed to read a camera frame.")
                break

            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            gray = cv2.equalizeHist(gray)

            threshold = cv2.getTrackbarPos("Detection threshold", WINDOW_NAME)
            min_radius = cv2.getTrackbarPos("Minimum radius", WINDOW_NAME)
            max_radius = cv2.getTrackbarPos("Maximum radius", WINDOW_NAME)

            target, all_circles = find_target(
                gray,
                threshold,
                min_radius,
                max_radius,
                args.min_rings,
            )

            # Draw raw Hough candidates in gray so parameter tuning is visible.
            for x, y, radius in all_circles:
                cv2.circle(frame, (x, y), radius, (120, 120, 120), 1)

            if target is not None:
                x, y, radius, ring_count = target
                missing_frames = 0
                if smoothed_x is None:
                    smoothed_x, smoothed_y, smoothed_radius = x, y, radius
                else:
                    smoothed_x += smoothing * (x - smoothed_x)
                    smoothed_y += smoothing * (y - smoothed_y)
                    smoothed_radius += smoothing * (radius - smoothed_radius)

                draw_x = int(smoothed_x)
                draw_y = int(smoothed_y)
                draw_radius = int(smoothed_radius)
                cv2.circle(frame, (draw_x, draw_y), draw_radius, (0, 255, 0), 3)
                cv2.drawMarker(
                    frame,
                    (draw_x, draw_y),
                    (0, 0, 255),
                    cv2.MARKER_CROSS,
                    24,
                    2,
                )
                status = (
                    f"TARGET x={draw_x} y={draw_y} "
                    f"r={draw_radius} rings={ring_count}"
                )
                status_color = (0, 255, 0)
            else:
                missing_frames += 1
                if missing_frames > 10:
                    smoothed_x = smoothed_y = smoothed_radius = None
                status = "NO CONCENTRIC TARGET"
                status_color = (0, 0, 255)

            current_time = time.perf_counter()
            fps = 1.0 / max(current_time - previous_time, 1e-6)
            previous_time = current_time

            cv2.putText(
                frame,
                status,
                (12, 30),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.7,
                status_color,
                2,
            )
            cv2.putText(
                frame,
                f"FPS {fps:.1f} | Q/Esc: quit",
                (12, 58),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.6,
                (255, 255, 0),
                2,
            )

            cv2.imshow(WINDOW_NAME, frame)
            key = cv2.waitKey(1) & 0xFF
            if key in (ord("q"), 27):
                break
    finally:
        capture.release()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
