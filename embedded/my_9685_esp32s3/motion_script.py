from __future__ import annotations

from dataclasses import dataclass
import json
import math
from pathlib import Path
from typing import Callable, Dict, Iterable, List, Optional, Tuple


def round_0p1(x: float) -> float:
    x = float(x)
    if x >= 0:
        return math.floor(x * 10.0 + 0.5) / 10.0
    return math.ceil(x * 10.0 - 0.5) / 10.0


def clamp(x: float, lo: float, hi: float) -> float:
    if x < lo:
        return lo
    if x > hi:
        return hi
    return x


Angles = Dict[int, float]


@dataclass(frozen=True)
class ScriptStep:
    index: int
    angles: Angles
    event: Optional[str] = None


@dataclass(frozen=True)
class MotionScript:
    version: int
    channels: List[int]
    steps: List[ScriptStep]
    meta: Dict[str, object]


class ScriptFormatError(ValueError):
    pass


def filter_angles(angles: Angles, channels: Iterable[int]) -> Angles:
    allowed = set(int(c) for c in channels)
    return {int(k): round_0p1(v) for k, v in angles.items() if int(k) in allowed}


def serialize_script(script: MotionScript) -> str:
    data = {
        "version": script.version,
        "channels": list(map(int, script.channels)),
        "meta": script.meta,
        "steps": [
            {
                "index": int(s.index),
                "angles": {str(int(k)): round_0p1(v) for k, v in s.angles.items()},
                **({} if s.event is None else {"event": str(s.event)}),
            }
            for s in script.steps
        ],
    }
    return json.dumps(data, ensure_ascii=False, indent=2)


def parse_script(text: str) -> MotionScript:
    try:
        raw = json.loads(text)
    except Exception as e:
        raise ScriptFormatError(f"invalid json: {e}") from e

    if not isinstance(raw, dict):
        raise ScriptFormatError("root must be an object")

    version = int(raw.get("version", 1))
    channels_raw = raw.get("channels")
    if not isinstance(channels_raw, list) or not channels_raw:
        raise ScriptFormatError("channels must be a non-empty array")
    channels = [int(x) for x in channels_raw]

    meta = raw.get("meta", {})
    if not isinstance(meta, dict):
        raise ScriptFormatError("meta must be an object")

    steps_raw = raw.get("steps")
    if not isinstance(steps_raw, list) or not steps_raw:
        raise ScriptFormatError("steps must be a non-empty array")

    steps: List[ScriptStep] = []
    for i, item in enumerate(steps_raw):
        if not isinstance(item, dict):
            raise ScriptFormatError(f"step[{i}] must be an object")
        index = int(item.get("index", i + 1))
        angles_raw = item.get("angles")
        if not isinstance(angles_raw, dict) or not angles_raw:
            raise ScriptFormatError(f"step[{i}].angles must be an object")
        angles: Angles = {}
        for k, v in angles_raw.items():
            try:
                ch = int(k)
            except Exception:
                raise ScriptFormatError(f"step[{i}].angles key must be int-like, got {k!r}")
            try:
                ang = float(v)
            except Exception:
                raise ScriptFormatError(f"step[{i}].angles[{k}] must be number-like")
            angles[ch] = round_0p1(ang)

        event = item.get("event")
        if event is not None:
            event = str(event)

        steps.append(ScriptStep(index=index, angles=angles, event=event))

    steps.sort(key=lambda s: s.index)
    return MotionScript(version=version, channels=channels, steps=steps, meta=meta)


def save_script_file(script: MotionScript, path: Path) -> None:
    path.write_text(serialize_script(script), encoding="utf-8")


def load_script_file(path: Path) -> MotionScript:
    return parse_script(path.read_text(encoding="utf-8"))


def linear_frames(start: Angles, target: Angles, duration_s: float, hz: float) -> List[Angles]:
    duration_s = float(duration_s)
    hz = float(hz)
    if duration_s <= 0:
        return [dict(target)]
    if hz <= 0:
        return [dict(target)]

    steps = max(1, int(math.ceil(duration_s * hz)))
    frames: List[Angles] = []
    for i in range(1, steps + 1):
        t = i / float(steps)
        frame: Angles = {}
        keys = set(start.keys()) | set(target.keys())
        for ch in keys:
            a0 = float(start.get(ch, target.get(ch, 0.0)))
            a1 = float(target.get(ch, start.get(ch, 0.0)))
            v = a0 + (a1 - a0) * t
            frame[int(ch)] = round_0p1(v)
        frames.append(frame)
    return frames


class MotionSegmenter:
    def __init__(
        self,
        *,
        stable_window_s: float = 0.35,
        stable_eps_deg: float = 0.2,
        min_step_change_deg: float = 0.8,
        max_steps: int = 15,
    ):
        self._stable_window_s = float(stable_window_s)
        self._stable_eps_deg = float(stable_eps_deg)
        self._min_step_change_deg = float(min_step_change_deg)
        self._max_steps = int(max_steps)

        self._samples: List[Tuple[float, Angles]] = []
        self._steps: List[ScriptStep] = []
        self._last_committed: Optional[Angles] = None
        self._pending_event: Optional[str] = None

    @property
    def steps(self) -> List[ScriptStep]:
        return list(self._steps)

    def set_pending_event(self, event: str) -> None:
        self._pending_event = str(event)

    def add_sample(self, t_s: float, angles: Angles) -> None:
        if len(self._steps) >= self._max_steps:
            return

        angles = {int(k): round_0p1(v) for k, v in angles.items()}
        self._samples.append((float(t_s), angles))

        cutoff = t_s - self._stable_window_s
        while self._samples and self._samples[0][0] < cutoff:
            self._samples.pop(0)

        if not self._samples:
            return

        if not self._is_window_stable(self._samples):
            return

        avg = self._avg_angles([a for _t, a in self._samples])
        if self._last_committed is not None:
            if self._max_abs_delta(self._last_committed, avg) < self._min_step_change_deg and self._pending_event is None:
                return

        step_index = len(self._steps) + 1
        ev = self._pending_event
        self._pending_event = None
        self._steps.append(ScriptStep(index=step_index, angles=avg, event=ev))
        self._last_committed = avg
        self._samples.clear()

    def flush_final(self, t_s: float, angles: Angles) -> None:
        if len(self._steps) >= self._max_steps:
            return
        angles = {int(k): round_0p1(v) for k, v in angles.items()}
        if self._last_committed is None or self._max_abs_delta(self._last_committed, angles) >= self._min_step_change_deg or self._pending_event is not None:
            step_index = len(self._steps) + 1
            ev = self._pending_event
            self._pending_event = None
            self._steps.append(ScriptStep(index=step_index, angles=angles, event=ev))
            self._last_committed = angles

    def _is_window_stable(self, window: List[Tuple[float, Angles]]) -> bool:
        if len(window) < 2:
            return False
        angles_list = [a for _t, a in window]
        avg = self._avg_angles(angles_list)
        for a in angles_list:
            if self._max_abs_delta(a, avg) > self._stable_eps_deg:
                return False
        return True

    def _avg_angles(self, angles_list: List[Angles]) -> Angles:
        keys = set()
        for a in angles_list:
            keys |= set(a.keys())
        out: Angles = {}
        for ch in keys:
            s = 0.0
            n = 0
            for a in angles_list:
                if ch in a:
                    s += float(a[ch])
                    n += 1
            if n > 0:
                out[int(ch)] = round_0p1(s / float(n))
        return out

    def _max_abs_delta(self, a: Angles, b: Angles) -> float:
        keys = set(a.keys()) | set(b.keys())
        m = 0.0
        for ch in keys:
            m = max(m, abs(float(a.get(ch, 0.0)) - float(b.get(ch, 0.0))))
        return m


class PlaybackError(RuntimeError):
    def __init__(self, code: str, message: str):
        super().__init__(message)
        self.code = code


def validate_angles(
    angles: Angles,
    *,
    lo: float = 0.0,
    hi: float = 180.0,
    required_channels: Optional[Iterable[int]] = None,
) -> Tuple[Angles, List[str]]:
    errors: List[str] = []
    out: Angles = {}
    req = set(int(c) for c in required_channels) if required_channels is not None else None
    for ch, v in angles.items():
        ch = int(ch)
        fv = float(v)
        if fv < lo or fv > hi:
            errors.append(f"CH{ch} out of range: {fv}")
        out[ch] = round_0p1(clamp(fv, lo, hi))
    if req is not None:
        missing = sorted(req - set(out.keys()))
        if missing:
            errors.append(f"missing channels: {missing}")
    return out, errors


def default_click_callback(step: ScriptStep) -> None:
    _ = step
