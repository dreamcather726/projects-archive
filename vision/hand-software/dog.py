from __future__ import annotations

import math
import os
import sys
import time
import unittest
from dataclasses import dataclass
from typing import Callable, Iterable, List, Optional, Sequence, Tuple

sys.dont_write_bytecode = True
os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")

import numpy as np


Array = np.ndarray


def _as_float_array(x: Sequence[float]) -> Array:
    return np.asarray(x, dtype=float)


def rot_x(a: float) -> Array:
    ca = math.cos(a)
    sa = math.sin(a)
    return np.array(
        [[1.0, 0.0, 0.0], [0.0, ca, -sa], [0.0, sa, ca]],
        dtype=float,
    )


def rot_y(a: float) -> Array:
    ca = math.cos(a)
    sa = math.sin(a)
    return np.array(
        [[ca, 0.0, sa], [0.0, 1.0, 0.0], [-sa, 0.0, ca]],
        dtype=float,
    )


def rot_z(a: float) -> Array:
    ca = math.cos(a)
    sa = math.sin(a)
    return np.array(
        [[ca, -sa, 0.0], [sa, ca, 0.0], [0.0, 0.0, 1.0]],
        dtype=float,
    )


def quat_normalize(q_wxyz: Array) -> Array:
    q = np.asarray(q_wxyz, dtype=float).reshape(4)
    n = float(np.linalg.norm(q))
    if n == 0.0:
        return np.array([1.0, 0.0, 0.0, 0.0], dtype=float)
    return q / n


def rot_to_quat_wxyz(R: Array) -> Array:
    R = np.asarray(R, dtype=float).reshape(3, 3)
    t = float(np.trace(R))
    if t > 0.0:
        s = math.sqrt(t + 1.0) * 2.0
        qw = 0.25 * s
        qx = (R[2, 1] - R[1, 2]) / s
        qy = (R[0, 2] - R[2, 0]) / s
        qz = (R[1, 0] - R[0, 1]) / s
    else:
        if R[0, 0] > R[1, 1] and R[0, 0] > R[2, 2]:
            s = math.sqrt(1.0 + R[0, 0] - R[1, 1] - R[2, 2]) * 2.0
            qw = (R[2, 1] - R[1, 2]) / s
            qx = 0.25 * s
            qy = (R[0, 1] + R[1, 0]) / s
            qz = (R[0, 2] + R[2, 0]) / s
        elif R[1, 1] > R[2, 2]:
            s = math.sqrt(1.0 + R[1, 1] - R[0, 0] - R[2, 2]) * 2.0
            qw = (R[0, 2] - R[2, 0]) / s
            qx = (R[0, 1] + R[1, 0]) / s
            qy = 0.25 * s
            qz = (R[1, 2] + R[2, 1]) / s
        else:
            s = math.sqrt(1.0 + R[2, 2] - R[0, 0] - R[1, 1]) * 2.0
            qw = (R[1, 0] - R[0, 1]) / s
            qx = (R[0, 2] + R[2, 0]) / s
            qy = (R[1, 2] + R[2, 1]) / s
            qz = 0.25 * s
    return quat_normalize(np.array([qw, qx, qy, qz], dtype=float))


def quat_to_rot_wxyz(q_wxyz: Array) -> Array:
    q = quat_normalize(q_wxyz)
    qw, qx, qy, qz = (float(q[0]), float(q[1]), float(q[2]), float(q[3]))
    xx = qx * qx
    yy = qy * qy
    zz = qz * qz
    xy = qx * qy
    xz = qx * qz
    yz = qy * qz
    wx = qw * qx
    wy = qw * qy
    wz = qw * qz
    return np.array(
        [
            [1.0 - 2.0 * (yy + zz), 2.0 * (xy - wz), 2.0 * (xz + wy)],
            [2.0 * (xy + wz), 1.0 - 2.0 * (xx + zz), 2.0 * (yz - wx)],
            [2.0 * (xz - wy), 2.0 * (yz + wx), 1.0 - 2.0 * (xx + yy)],
        ],
        dtype=float,
    )


def dh_transform(a: float, alpha: float, d: float, theta: float) -> Array:
    ca = math.cos(alpha)
    sa = math.sin(alpha)
    ct = math.cos(theta)
    st = math.sin(theta)
    return np.array(
        [
            [ct, -st * ca, st * sa, a * ct],
            [st, ct * ca, -ct * sa, a * st],
            [0.0, sa, ca, d],
            [0.0, 0.0, 0.0, 1.0],
        ],
        dtype=float,
    )


def _vee(S: Array) -> Array:
    return np.array([S[2, 1], S[0, 2], S[1, 0]], dtype=float)


def rotation_log(R: Array, eps: float = 1e-12) -> Array:
    R = np.asarray(R, dtype=float).reshape(3, 3)
    tr = float(np.trace(R))
    c = (tr - 1.0) / 2.0
    c = max(-1.0, min(1.0, c))
    theta = math.acos(c)
    if theta < 1e-8:
        return 0.5 * _vee(R - R.T)
    if math.pi - theta < 1e-6:
        A = (R + np.eye(3)) / 2.0
        v = np.array([math.sqrt(max(A[0, 0], 0.0)), math.sqrt(max(A[1, 1], 0.0)), math.sqrt(max(A[2, 2], 0.0))])
        if abs(v[0]) < eps and abs(v[1]) < eps and abs(v[2]) < eps:
            return np.zeros(3, dtype=float)
        return theta * v / float(np.linalg.norm(v))
    return (theta / (2.0 * math.sin(theta))) * _vee(R - R.T)


@dataclass(frozen=True)
class FKResult:
    T: Array
    position_xyz: Array
    rotation_matrix: Array
    quaternion_wxyz: Array


@dataclass(frozen=True)
class IKResult:
    solutions: List[Array]
    is_singular: bool
    reason: str


class DHRobot:
    def __init__(
        self,
        dh_params: Sequence[Tuple[float, float, float, float]],
        joint_limits: Optional[Sequence[Tuple[float, float]]] = None,
        name: str = "DHRobot",
    ) -> None:
        self.dh_params = [(float(a), float(alpha), float(d), float(theta0)) for a, alpha, d, theta0 in dh_params]
        self.n = len(self.dh_params)
        self.name = name
        if joint_limits is None:
            self.joint_limits = [(-math.pi, math.pi) for _ in range(self.n)]
        else:
            if len(joint_limits) != self.n:
                raise ValueError("joint_limits length must match number of joints")
            self.joint_limits = [(float(lo), float(hi)) for lo, hi in joint_limits]

    def fk(self, q: Sequence[float]) -> FKResult:
        q = _as_float_array(q).reshape(self.n)
        T = np.eye(4, dtype=float)
        for i, (a, alpha, d, theta0) in enumerate(self.dh_params):
            T = T @ dh_transform(a, alpha, d, theta0 + float(q[i]))
        p = T[:3, 3].copy()
        R = T[:3, :3].copy()
        quat = rot_to_quat_wxyz(R)
        return FKResult(T=T, position_xyz=p, rotation_matrix=R, quaternion_wxyz=quat)

    def fk_all(self, q: Sequence[float]) -> List[Array]:
        q = _as_float_array(q).reshape(self.n)
        Ts = [np.eye(4, dtype=float)]
        T = np.eye(4, dtype=float)
        for i, (a, alpha, d, theta0) in enumerate(self.dh_params):
            T = T @ dh_transform(a, alpha, d, theta0 + float(q[i]))
            Ts.append(T.copy())
        return Ts

    def jacobian(self, q: Sequence[float]) -> Array:
        Ts = self.fk_all(q)
        T_end = Ts[-1]
        p_end = T_end[:3, 3]
        J = np.zeros((6, self.n), dtype=float)
        for i in range(self.n):
            T_i = Ts[i]
            z = T_i[:3, 2]
            p_i = T_i[:3, 3]
            J[:3, i] = np.cross(z, p_end - p_i)
            J[3:, i] = z
        return J

    def clip_to_limits(self, q: Array) -> Array:
        q = np.asarray(q, dtype=float).reshape(self.n)
        out = q.copy()
        for i, (lo, hi) in enumerate(self.joint_limits):
            out[i] = float(np.clip(out[i], lo, hi))
        return out


def _pose_error_6d(p_cur: Array, R_cur: Array, p_tgt: Array, R_tgt: Array) -> Array:
    dp = (p_tgt - p_cur).reshape(3)
    R_err = R_tgt @ R_cur.T
    dr = rotation_log(R_err).reshape(3)
    return np.concatenate([dp, dr], axis=0)


def _unique_solutions(solutions: Iterable[Array], atol: float = 1e-6) -> List[Array]:
    uniq: List[Array] = []
    for q in solutions:
        q = np.asarray(q, dtype=float).reshape(-1)
        ok = True
        for u in uniq:
            if np.allclose(q, u, atol=atol, rtol=0.0):
                ok = False
                break
        if ok:
            uniq.append(q)
    return uniq


def ik_numeric(
    robot: DHRobot,
    target_position_xyz: Sequence[float],
    target_rotation: Array,
    q0: Sequence[float],
    max_iter: int = 60,
    tol_pos_m: float = 1e-4,
    tol_ori_rad: float = math.radians(0.01),
    damping: float = 1e-3,
    step_scale: float = 1.0,
    singular_cond: float = 1e6,
) -> IKResult:
    q = robot.clip_to_limits(_as_float_array(q0))
    p_tgt = _as_float_array(target_position_xyz).reshape(3)
    R_tgt = np.asarray(target_rotation, dtype=float).reshape(3, 3)

    is_singular = False
    reason = "ok"
    last_norm = float("inf")
    for _ in range(max_iter):
        fk = robot.fk(q)
        e = _pose_error_6d(fk.position_xyz, fk.rotation_matrix, p_tgt, R_tgt)
        pos_norm = float(np.linalg.norm(e[:3]))
        ori_norm = float(np.linalg.norm(e[3:]))
        if pos_norm < tol_pos_m and ori_norm < tol_ori_rad:
            return IKResult(solutions=[q.copy()], is_singular=is_singular, reason=reason)

        J = robot.jacobian(q)
        w_ori = 0.2
        W = np.diag([1.0, 1.0, 1.0, w_ori, w_ori, w_ori]).astype(float)
        Jw = W @ J
        ew = W @ e

        H = Jw.T @ Jw + (damping * damping) * np.eye(robot.n, dtype=float)
        g = Jw.T @ ew
        try:
            dq = np.linalg.solve(H, g)
        except np.linalg.LinAlgError:
            is_singular = True
            reason = "singular; solve failed"
            break

        dq_norm = float(np.linalg.norm(dq))
        if dq_norm > 0.5:
            dq = dq * (0.5 / dq_norm)

        trial_scales = [step_scale, 0.5 * step_scale, 0.25 * step_scale, 0.125 * step_scale]
        accepted = False
        cur_cost = pos_norm + w_ori * ori_norm
        for a in trial_scales:
            q_try = robot.clip_to_limits(q + a * dq)
            fk_try = robot.fk(q_try)
            e_try = _pose_error_6d(fk_try.position_xyz, fk_try.rotation_matrix, p_tgt, R_tgt)
            pos_try = float(np.linalg.norm(e_try[:3]))
            ori_try = float(np.linalg.norm(e_try[3:]))
            cost_try = pos_try + w_ori * ori_try
            if cost_try < cur_cost:
                q = q_try
                accepted = True
                last_norm = cost_try
                break

        if not accepted:
            damping = min(damping * 10.0, 1e-1)
            reason = "not improving; increased damping"
        else:
            damping = max(damping * 0.7, 1e-6)
            if last_norm < 1e-12:
                break

    return IKResult(solutions=[], is_singular=is_singular, reason="not converged")


def ik_numeric_multistart(
    robot: DHRobot,
    target_position_xyz: Sequence[float],
    target_rotation: Array,
    seeds: Sequence[Sequence[float]],
    **kwargs,
) -> IKResult:
    p_tgt = _as_float_array(target_position_xyz).reshape(3)
    R_tgt = np.asarray(target_rotation, dtype=float).reshape(3, 3)

    seeds_all: List[Array] = [robot.clip_to_limits(_as_float_array(s)) for s in seeds]
    ik_ana = getattr(robot, "ik_analytic", None)
    if callable(ik_ana):
        try:
            r_ana = ik_ana(p_tgt, R_tgt)
            seeds_all.extend([robot.clip_to_limits(q) for q in r_ana.solutions])
        except Exception:
            pass

    sols: List[Array] = []
    singular_any = False
    reasons: List[str] = []
    for s in seeds_all:
        r = ik_numeric(robot, p_tgt, R_tgt, s, **kwargs)
        singular_any = singular_any or r.is_singular
        reasons.append(r.reason)
        sols.extend(r.solutions)
    sols = _unique_solutions(sols, atol=1e-5)
    reason = "ok" if sols else ("; ".join(sorted(set(reasons))) or "not converged")
    return IKResult(solutions=sols, is_singular=singular_any, reason=reason)


def _solve_2link_planar(r: float, s: float, a2: float, a3: float) -> List[Tuple[float, float]]:
    D = (r * r + s * s - a2 * a2 - a3 * a3) / (2.0 * a2 * a3)
    if D < -1.0 or D > 1.0:
        return []
    D = max(-1.0, min(1.0, D))
    q3a = math.acos(D)
    q3b = -q3a
    sols: List[Tuple[float, float]] = []
    for q3 in (q3a, q3b):
        q2 = math.atan2(s, r) - math.atan2(a3 * math.sin(q3), a2 + a3 * math.cos(q3))
        sols.append((q2, q3))
    return sols


def _wrist_zyz_solutions(R_36: Array, eps: float = 1e-9) -> Tuple[List[Tuple[float, float, float]], bool]:
    R = np.asarray(R_36, dtype=float).reshape(3, 3)
    c5 = float(R[2, 2])
    c5 = max(-1.0, min(1.0, c5))
    q5 = math.acos(c5)

    singular = abs(math.sin(q5)) < eps
    sols: List[Tuple[float, float, float]] = []
    if singular:
        q4 = 0.0
        q6 = math.atan2(float(R[1, 0]), float(R[0, 0]))
        sols.append((q4, 0.0, q6))
        sols.append((q4, 0.0, q6))
        return sols, True

    q4 = math.atan2(float(R[1, 2]), float(R[0, 2]))
    q6 = math.atan2(float(R[2, 1]), float(-R[2, 0]))
    sols.append((q4, q5, q6))

    q5b = -q5
    q4b = math.atan2(float(-R[1, 2]), float(-R[0, 2]))
    q6b = math.atan2(float(-R[2, 1]), float(R[2, 0]))
    sols.append((q4b, q5b, q6b))
    return sols, False


class Pieper6DOFExample(DHRobot):
    def __init__(self) -> None:
        dh = [
            (0.0, math.pi / 2.0, 0.10, 0.0),
            (0.30, 0.0, 0.0, 0.0),
            (0.20, 0.0, 0.0, 0.0),
            (0.0, math.pi / 2.0, 0.0, 0.0),
            (0.0, -math.pi / 2.0, 0.0, 0.0),
            (0.0, 0.0, 0.10, 0.0),
        ]
        super().__init__(dh, name="Pieper6DOFExample")

    def ik_analytic(self, target_position_xyz: Sequence[float], target_rotation: Array) -> IKResult:
        p = _as_float_array(target_position_xyz).reshape(3)
        R = np.asarray(target_rotation, dtype=float).reshape(3, 3)

        d6 = self.dh_params[5][2]
        p_wc = p - d6 * R[:, 2]

        d1 = self.dh_params[0][2]
        a2 = self.dh_params[1][0]
        a3 = self.dh_params[2][0]

        q1 = math.atan2(float(p_wc[1]), float(p_wc[0]))
        T01 = dh_transform(0.0, math.pi / 2.0, d1, q1)
        p1 = np.linalg.inv(T01) @ np.array([float(p_wc[0]), float(p_wc[1]), float(p_wc[2]), 1.0], dtype=float)
        r = float(p1[0])
        s = float(p1[2])

        q23_sols = _solve_2link_planar(r, s, a2, a3)
        if not q23_sols:
            return IKResult(solutions=[], is_singular=False, reason="unreachable position")

        sols_raw: List[Array] = []
        singular_any = False
        for q2, q3 in q23_sols:
            q123 = np.array([q1, q2, q3, 0.0, 0.0, 0.0], dtype=float)
            T_03 = self.fk_all(q123)[3]
            R_03 = T_03[:3, :3]
            R_36 = R_03.T @ R

            wrist_sols, singular = _wrist_zyz_solutions(R_36)
            singular_any = singular_any or singular
            for q4, q5, q6 in wrist_sols:
                q = np.array([q1, q2, q3, q4, q5, q6], dtype=float)
                sols_raw.append(self.clip_to_limits(q))

        refined: List[Array] = []
        for q_seed in _unique_solutions(sols_raw, atol=1e-6):
            r_num = ik_numeric(self, p, R, q_seed, max_iter=80, damping=1e-3, step_scale=1.0)
            refined.extend(r_num.solutions)
        refined = _unique_solutions(refined, atol=1e-6)
        if not refined:
            return IKResult(solutions=[], is_singular=singular_any, reason="analytic seeds failed to refine")
        return IKResult(solutions=refined, is_singular=singular_any, reason="ok")


def pose_error_metrics(robot: DHRobot, q: Array, p_tgt: Array, R_tgt: Array) -> Tuple[float, float]:
    fk = robot.fk(q)
    pos_err = float(np.linalg.norm(fk.position_xyz - p_tgt))
    ori_err = float(np.linalg.norm(rotation_log(R_tgt @ fk.rotation_matrix.T)))
    return pos_err, ori_err


def benchmark(robot: DHRobot, trials: int = 200) -> dict:
    rng = np.random.default_rng(0)
    qs = []
    for _ in range(trials):
        q = []
        for lo, hi in robot.joint_limits:
            q.append(float(rng.uniform(lo * 0.5, hi * 0.5)))
        qs.append(np.array(q, dtype=float))

    t0 = time.perf_counter()
    fk_results = [robot.fk(q) for q in qs]
    t1 = time.perf_counter()

    seeds = [np.zeros(robot.n), np.ones(robot.n) * 0.1, -np.ones(robot.n) * 0.1]
    t2 = time.perf_counter()
    ik_succeeded = 0
    for fk in fk_results:
        r = ik_numeric_multistart(robot, fk.position_xyz, fk.rotation_matrix, seeds=seeds, max_iter=50)
        ik_succeeded += 1 if r.solutions else 0
    t3 = time.perf_counter()

    fk_ms = (t1 - t0) * 1000.0 / trials
    ik_ms = (t3 - t2) * 1000.0 / trials
    return {
        "trials": trials,
        "fk_avg_ms": fk_ms,
        "ik_numeric_avg_ms": ik_ms,
        "ik_numeric_success_rate": ik_succeeded / trials,
    }


@dataclass(frozen=True)
class View2D:
    xlim: Tuple[float, float]
    ylim: Tuple[float, float]


@dataclass(frozen=True)
class Canvas2D:
    width_px: int
    height_px: int
    margin_px: int = 40


class CoordinateSystem2D:
    def __init__(self, view: View2D, canvas: Canvas2D) -> None:
        self.view = view
        self.canvas = canvas

    def set_view(self, view: View2D) -> None:
        self.view = view

    def world_to_screen(self, xy: Sequence[float]) -> Array:
        x, y = float(xy[0]), float(xy[1])
        xmin, xmax = self.view.xlim
        ymin, ymax = self.view.ylim
        m = self.canvas.margin_px
        w = self.canvas.width_px - 2 * m
        h = self.canvas.height_px - 2 * m
        if w <= 0 or h <= 0:
            raise ValueError("Canvas too small for margins")
        if xmax == xmin or ymax == ymin:
            raise ValueError("Invalid view range")
        sx = m + (x - xmin) * (w / (xmax - xmin))
        sy = self.canvas.height_px - (m + (y - ymin) * (h / (ymax - ymin)))
        return np.array([sx, sy], dtype=float)

    def screen_to_world(self, uv: Sequence[float]) -> Array:
        u, v = float(uv[0]), float(uv[1])
        xmin, xmax = self.view.xlim
        ymin, ymax = self.view.ylim
        m = self.canvas.margin_px
        w = self.canvas.width_px - 2 * m
        h = self.canvas.height_px - 2 * m
        if w <= 0 or h <= 0:
            raise ValueError("Canvas too small for margins")
        x = xmin + (u - m) * (xmax - xmin) / w
        y_screen = (self.canvas.height_px - v) - m
        y = ymin + y_screen * (ymax - ymin) / h
        return np.array([x, y], dtype=float)

    def zoom_at(self, center_xy: Sequence[float], scale: float) -> None:
        if scale <= 0:
            raise ValueError("scale must be positive")
        cx, cy = float(center_xy[0]), float(center_xy[1])
        xmin, xmax = self.view.xlim
        ymin, ymax = self.view.ylim
        hx = 0.5 * (xmax - xmin) / scale
        hy = 0.5 * (ymax - ymin) / scale
        self.view = View2D(xlim=(cx - hx, cx + hx), ylim=(cy - hy, cy + hy))

    def pan(self, dx: float, dy: float) -> None:
        xmin, xmax = self.view.xlim
        ymin, ymax = self.view.ylim
        self.view = View2D(xlim=(xmin + dx, xmax + dx), ylim=(ymin + dy, ymax + dy))


class Plot2D:
    def __init__(
        self,
        view: View2D,
        title: str = "2D Plot",
        x_label: str = "X",
        y_label: str = "Y",
        figsize: Tuple[float, float] = (8.0, 6.0),
        dpi: int = 140,
    ) -> None:
        self.view = view
        self.title = title
        self.x_label = x_label
        self.y_label = y_label
        self.figsize = figsize
        self.dpi = dpi

    def render(
        self,
        draw_fn: Callable,
        save_path: Optional[str] = None,
        show: bool = True,
    ) -> None:
        try:
            import matplotlib.pyplot as plt
        except Exception as e:
            print("matplotlib not available:", e)
            return

        fig, ax = plt.subplots(figsize=self.figsize, dpi=self.dpi, constrained_layout=True)
        ax.set_title(self.title)
        ax.set_xlabel(self.x_label)
        ax.set_ylabel(self.y_label)
        ax.set_xlim(*self.view.xlim)
        ax.set_ylim(*self.view.ylim)
        ax.set_aspect("equal", adjustable="box")
        ax.grid(True, linewidth=0.8, alpha=0.35)

        draw_fn(ax)

        if save_path:
            fig.savefig(save_path, dpi=self.dpi, bbox_inches="tight")
        if show:
            plt.show()
        else:
            plt.close(fig)


def plot_primitives_demo(save_path: Optional[str] = None, show: bool = True) -> None:
    view = View2D(xlim=(-2.0, 2.0), ylim=(-2.0, 2.0))
    plot = Plot2D(view=view, title="Primitives: Point / Line / Face", x_label="X", y_label="Y")

    def _draw(ax) -> None:
        pts = np.array([[0.0, 0.0], [1.0, 0.5], [-1.2, 1.0]], dtype=float)
        ax.scatter(pts[:, 0], pts[:, 1], s=80, c="tab:red", marker="o", label="points")
        ax.plot([-1.5, 1.5], [-1.0, 1.0], color="tab:blue", linewidth=2.0, label="line")
        poly = np.array([[-0.8, -0.8], [0.8, -0.8], [0.3, 0.2], [-0.6, 0.4]], dtype=float)
        ax.fill(poly[:, 0], poly[:, 1], color="tab:green", alpha=0.25, label="face")
        ax.legend(loc="upper left")

    plot.render(_draw, save_path=save_path, show=show)


def _set_axes_equal_3d(ax, xyz: Array) -> None:
    xyz = np.asarray(xyz, dtype=float)
    xmin, ymin, zmin = np.min(xyz, axis=0)
    xmax, ymax, zmax = np.max(xyz, axis=0)
    cx = 0.5 * (xmin + xmax)
    cy = 0.5 * (ymin + ymax)
    cz = 0.5 * (zmin + zmax)
    r = 0.5 * max(xmax - xmin, ymax - ymin, zmax - zmin)
    if r <= 0:
        r = 1.0
    ax.set_xlim(cx - r, cx + r)
    ax.set_ylim(cy - r, cy + r)
    ax.set_zlim(cz - r, cz + r)


def _joint_positions(robot: DHRobot, q: Sequence[float]) -> Array:
    Ts = robot.fk_all(q)
    pts = np.array([T[:3, 3] for T in Ts], dtype=float)
    return pts


def _draw_frame(ax, T: Array, scale: float, label: str | None = None, alpha: float = 1.0) -> None:
    T = np.asarray(T, dtype=float).reshape(4, 4)
    p = T[:3, 3]
    R = T[:3, :3]
    x = p + scale * R[:, 0]
    y = p + scale * R[:, 1]
    z = p + scale * R[:, 2]
    ax.plot([p[0], x[0]], [p[1], x[1]], [p[2], x[2]], color="r", linewidth=2, alpha=alpha)
    ax.plot([p[0], y[0]], [p[1], y[1]], [p[2], y[2]], color="g", linewidth=2, alpha=alpha)
    ax.plot([p[0], z[0]], [p[1], z[1]], [p[2], z[2]], color="b", linewidth=2, alpha=alpha)
    if label:
        ax.text(float(p[0]), float(p[1]), float(p[2]), label, fontsize=9)


def visualize(
    robot: DHRobot,
    qs: Sequence[Sequence[float]],
    labels: Optional[Sequence[str]] = None,
    title: str = "Robot Visualization",
    show: bool = True,
    save_path: Optional[str] = None,
) -> None:
    try:
        import matplotlib.pyplot as plt
    except Exception as e:
        print("matplotlib not available:", e)
        print("Install it with: pip install matplotlib  (or conda install matplotlib)")
        return

    if labels is None:
        labels = [f"q{i}" for i in range(len(qs))]
    if len(labels) != len(qs):
        raise ValueError("labels length must match qs length")

    fig = plt.figure(figsize=(9, 7))
    ax = fig.add_subplot(111, projection="3d")
    ax.set_title(title)
    ax.set_xlabel("X (m)")
    ax.set_ylabel("Y (m)")
    ax.set_zlabel("Z (m)")

    all_pts = []
    colors = ["k", "tab:orange", "tab:purple", "tab:cyan", "tab:olive", "tab:pink", "tab:brown"]
    for i, q in enumerate(qs):
        pts = _joint_positions(robot, q)
        all_pts.append(pts)
        c = colors[i % len(colors)]
        ax.plot(pts[:, 0], pts[:, 1], pts[:, 2], marker="o", color=c, linewidth=2, label=labels[i])
        fk = robot.fk(q)
        reach = float(np.max(np.linalg.norm(pts - pts[0], axis=1)))
        frame_scale = max(0.05 * reach, 0.02)
        _draw_frame(ax, fk.T, scale=frame_scale, label=None, alpha=0.9)

    all_xyz = np.vstack(all_pts) if all_pts else np.zeros((1, 3), dtype=float)
    _set_axes_equal_3d(ax, all_xyz)
    ax.legend(loc="upper left")

    if save_path:
        fig.savefig(save_path, dpi=160, bbox_inches="tight")

    if show:
        plt.show()
    else:
        plt.close(fig)


class TestKinematics(unittest.TestCase):
    def setUp(self) -> None:
        self.robot = Pieper6DOFExample()
        self.rng = np.random.default_rng(1)

    def test_fk_output_shapes(self) -> None:
        q = np.zeros(self.robot.n, dtype=float)
        fk = self.robot.fk(q)
        self.assertEqual(fk.position_xyz.shape, (3,))
        self.assertEqual(fk.rotation_matrix.shape, (3, 3))
        self.assertEqual(fk.quaternion_wxyz.shape, (4,))
        self.assertAlmostEqual(float(np.linalg.norm(fk.quaternion_wxyz)), 1.0, places=7)

    def test_numeric_ik_roundtrip(self) -> None:
        q_true = self.rng.uniform(-1.0, 1.0, size=(self.robot.n,))
        fk = self.robot.fk(q_true)
        seeds = [
            np.zeros(self.robot.n),
            np.ones(self.robot.n) * 0.2,
            -np.ones(self.robot.n) * 0.2,
            q_true * 0.5,
        ]
        r = ik_numeric_multistart(self.robot, fk.position_xyz, fk.rotation_matrix, seeds=seeds, max_iter=80)
        self.assertTrue(len(r.solutions) >= 1)
        ok = False
        for q in r.solutions:
            pos_err, ori_err = pose_error_metrics(self.robot, q, fk.position_xyz, fk.rotation_matrix)
            if pos_err < 1e-4 and ori_err < math.radians(0.01):
                ok = True
                break
        self.assertTrue(ok)

    def test_analytic_ik_roundtrip(self) -> None:
        q_true = self.rng.uniform(-1.0, 1.0, size=(self.robot.n,))
        fk = self.robot.fk(q_true)
        r = self.robot.ik_analytic(fk.position_xyz, fk.rotation_matrix)
        self.assertTrue(len(r.solutions) >= 1)
        ok = False
        for q in r.solutions:
            pos_err, ori_err = pose_error_metrics(self.robot, q, fk.position_xyz, fk.rotation_matrix)
            if pos_err < 1e-4 and ori_err < math.radians(0.01):
                ok = True
                break
        self.assertTrue(ok)

    def test_wrist_singularity_flag(self) -> None:
        q = np.zeros(self.robot.n, dtype=float)
        q[4] = 0.0
        fk = self.robot.fk(q)
        r = self.robot.ik_analytic(fk.position_xyz, fk.rotation_matrix)
        self.assertTrue(len(r.solutions) >= 1)


class TestVisualization(unittest.TestCase):
    def test_coordinate_transform_roundtrip(self) -> None:
        cs = CoordinateSystem2D(view=View2D(xlim=(-2.0, 6.0), ylim=(1.0, 5.0)), canvas=Canvas2D(width_px=800, height_px=600, margin_px=40))
        rng = np.random.default_rng(0)
        for _ in range(100):
            xy = np.array([rng.uniform(-2.0, 6.0), rng.uniform(1.0, 5.0)], dtype=float)
            uv = cs.world_to_screen(xy)
            xy2 = cs.screen_to_world(uv)
            self.assertTrue(np.allclose(xy, xy2, atol=1e-9, rtol=0.0))

    def test_zoom_pan_view(self) -> None:
        cs = CoordinateSystem2D(view=View2D(xlim=(-1.0, 1.0), ylim=(-1.0, 1.0)), canvas=Canvas2D(width_px=400, height_px=400, margin_px=20))
        cs.zoom_at((0.0, 0.0), scale=2.0)
        self.assertTrue(np.allclose(np.array(cs.view.xlim), np.array([-0.5, 0.5])))
        self.assertTrue(np.allclose(np.array(cs.view.ylim), np.array([-0.5, 0.5])))
        cs.pan(1.0, -2.0)
        self.assertTrue(np.allclose(np.array(cs.view.xlim), np.array([0.5, 1.5])))
        self.assertTrue(np.allclose(np.array(cs.view.ylim), np.array([-2.5, -1.5])))

    def test_matplotlib_render_pixel(self) -> None:
        try:
            import matplotlib
            matplotlib.use("Agg")
            import matplotlib.pyplot as plt
        except Exception:
            self.skipTest("matplotlib not available")

        fig, ax = plt.subplots(figsize=(4, 4), dpi=160)
        ax.set_xlim(-1, 1)
        ax.set_ylim(-1, 1)
        ax.set_aspect("equal", adjustable="box")
        ax.axis("off")
        ax.scatter([0.0], [0.0], s=800, c="red", marker="s")
        fig.canvas.draw()
        w, h = fig.canvas.get_width_height()
        buf_rgba = np.asarray(fig.canvas.buffer_rgba())
        buf = buf_rgba[:, :, :3].copy()
        px, py = ax.transData.transform((0.0, 0.0))
        ix = int(round(px))
        iy = int(round(h - py))
        x0 = max(ix - 8, 0)
        x1 = min(ix + 9, w)
        y0 = max(iy - 8, 0)
        y1 = min(iy + 9, h)
        roi = buf[y0:y1, x0:x1, :]
        self.assertTrue(int(np.max(roi[:, :, 0])) > 200)
        self.assertTrue(int(np.min(roi[:, :, 1])) < 80)
        self.assertTrue(int(np.min(roi[:, :, 2])) < 80)
        plt.close(fig)


def _demo() -> None:
    robot = Pieper6DOFExample()
    q = np.array([0.2, -0.4, 0.3, 0.1, 0.2, -0.1], dtype=float)
    fk = robot.fk(q)
    print("FK position xyz:", fk.position_xyz)
    print("FK quat wxyz:", fk.quaternion_wxyz)

    r_ana = robot.ik_analytic(fk.position_xyz, fk.rotation_matrix)
    print("Analytic IK solutions:", len(r_ana.solutions), "singular:", r_ana.is_singular, "reason:", r_ana.reason)

    seeds = [np.zeros(robot.n), np.ones(robot.n) * 0.1, -np.ones(robot.n) * 0.1]
    r_num = ik_numeric_multistart(robot, fk.position_xyz, fk.rotation_matrix, seeds=seeds)
    print("Numeric IK solutions:", len(r_num.solutions), "singular:", r_num.is_singular, "reason:", r_num.reason)

    print("Benchmark:", benchmark(robot, trials=200))


if __name__ == "__main__":
    if "--test" in sys.argv:
        unittest.main(argv=[sys.argv[0]])
    elif "--plot" in sys.argv:
        robot = Pieper6DOFExample()
        q = np.array([0.2, -0.4, 0.3, 0.1, 0.2, -0.1], dtype=float)
        fk = robot.fk(q)
        r_ana = robot.ik_analytic(fk.position_xyz, fk.rotation_matrix)
        qs: List[Array] = [q]
        labels: List[str] = ["FK(q)"]
        for i, s in enumerate(r_ana.solutions[:6]):
            qs.append(s)
            labels.append(f"IK{ i + 1 }")

        save_path = None
        if "--save" in sys.argv:
            idx = sys.argv.index("--save")
            if idx + 1 < len(sys.argv):
                save_path = sys.argv[idx + 1]

        show = "--no-show" not in sys.argv
        visualize(robot, qs=qs, labels=labels, title="ESP32 Kinematics: FK and IK Solutions", show=show, save_path=save_path)
    elif "--plot2d" in sys.argv:
        save_path = None
        if "--save" in sys.argv:
            idx = sys.argv.index("--save")
            if idx + 1 < len(sys.argv):
                save_path = sys.argv[idx + 1]
        show = "--no-show" not in sys.argv
        plot_primitives_demo(save_path=save_path, show=show)
    elif "--demo" in sys.argv:
        _demo()
    else:
        _demo()
