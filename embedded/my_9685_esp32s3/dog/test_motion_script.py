import unittest

from motion_script import (
    filter_angles,
    MotionSegmenter,
    MotionScript,
    ScriptStep,
    ScriptFormatError,
    linear_frames,
    parse_script,
    round_0p1,
    serialize_script,
    validate_angles,
)


class TestRounding(unittest.TestCase):
    def test_round_0p1(self):
        self.assertEqual(round_0p1(1.04), 1.0)
        self.assertEqual(round_0p1(1.05), 1.1)
        self.assertEqual(round_0p1(-0.04), 0.0)


class TestScriptIO(unittest.TestCase):
    def test_round_trip(self):
        script = MotionScript(
            version=1,
            channels=[0, 1, 2],
            steps=[
                ScriptStep(index=1, angles={0: 70.1, 1: 90.0, 2: 110.2}),
                ScriptStep(index=2, angles={0: 72.0, 1: 91.2, 2: 109.9}, event="click"),
            ],
            meta={"created_at": "20260101_000000"},
        )
        text = serialize_script(script)
        loaded = parse_script(text)
        self.assertEqual(loaded.version, 1)
        self.assertEqual(loaded.channels, [0, 1, 2])
        self.assertEqual(len(loaded.steps), 2)
        self.assertEqual(loaded.steps[1].event, "click")
        self.assertAlmostEqual(loaded.steps[0].angles[0], 70.1, places=1)

    def test_parse_errors(self):
        with self.assertRaises(ScriptFormatError):
            parse_script("not json")
        with self.assertRaises(ScriptFormatError):
            parse_script("{}")
        with self.assertRaises(ScriptFormatError):
            parse_script('{"channels":[0],"steps":[]}')


class TestInterpolation(unittest.TestCase):
    def test_linear_frames_count(self):
        frames = linear_frames({0: 0.0}, {0: 10.0}, duration_s=0.5, hz=20.0)
        self.assertEqual(len(frames), 10)
        self.assertAlmostEqual(frames[-1][0], 10.0, places=1)

    def test_linear_frames_monotonic(self):
        frames = linear_frames({0: 0.0}, {0: 5.0}, duration_s=1.0, hz=10.0)
        vals = [f[0] for f in frames]
        self.assertTrue(all(vals[i] <= vals[i + 1] for i in range(len(vals) - 1)))


class TestValidateAngles(unittest.TestCase):
    def test_validate_clamp_and_missing(self):
        out, errors = validate_angles({0: -10, 1: 181}, required_channels=[0, 1, 2])
        self.assertEqual(out[0], 0.0)
        self.assertEqual(out[1], 180.0)
        self.assertTrue(any("missing channels" in e for e in errors))


class TestFilterAngles(unittest.TestCase):
    def test_filter_angles(self):
        out = filter_angles({0: 10.04, 1: 20.05, 2: 30.0}, channels=[0, 2])
        self.assertEqual(out, {0: 10.0, 2: 30.0})


class TestSegmenter(unittest.TestCase):
    def test_segmenter_commits_on_stable(self):
        seg = MotionSegmenter(stable_window_s=0.2, stable_eps_deg=0.1, min_step_change_deg=0.5, max_steps=15)
        for i in range(10):
            seg.add_sample(i * 0.05, {0: 90.0, 1: 90.0})
        self.assertEqual(len(seg.steps), 1)
        self.assertEqual(seg.steps[0].index, 1)

    def test_segmenter_event(self):
        seg = MotionSegmenter(stable_window_s=0.2, stable_eps_deg=0.1, min_step_change_deg=0.5, max_steps=15)
        seg.set_pending_event("click")
        for i in range(10):
            seg.add_sample(i * 0.05, {0: 90.0})
        self.assertEqual(len(seg.steps), 1)
        self.assertEqual(seg.steps[0].event, "click")


if __name__ == "__main__":
    unittest.main()
