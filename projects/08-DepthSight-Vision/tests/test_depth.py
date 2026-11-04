"""Checks for depth validity, frame registration and fixed color scaling."""
import unittest

import numpy as np

from camera_depth import colorize_depth, parse_args, render_view, sample_depth


class DepthTests(unittest.TestCase):
    def test_patch_rejects_invalid_values_and_uses_median(self):
        values = np.array([[1, 2, 3], [np.nan, 0, -1], [np.inf, 4, 100]], np.float32)
        self.assertEqual(sample_depth(values, 1, 1), 3)
        self.assertIsNone(sample_depth(np.zeros((2, 2)), 0, 0))
        self.assertIsNone(sample_depth(values, -1, 0))

    def test_edges_do_not_wrap_around(self):
        values = np.full((20, 20), 100.0)
        values[:5, :5] = 2
        self.assertEqual(sample_depth(values, 0, 0), 2)

    def test_same_distance_has_same_color_across_frames(self):
        first = colorize_depth(np.array([[1, 2]], np.float32), 0.2, 10)
        second = colorize_depth(np.array([[1, 100]], np.float32), 0.2, 10)
        np.testing.assert_array_equal(first[0, 0], second[0, 0])
        invalid = colorize_depth(np.array([[np.nan, np.inf, 0, -1]], np.float32), 0.2, 10)
        self.assertFalse(invalid.any())

    def test_render_preserves_panel_geometry(self):
        rgb = np.zeros((480, 640, 3), np.uint8)
        view = render_view(rgb, np.ones((480, 640), np.float32), (0, 0), 10, "cpu", 0.2, 10)
        self.assertEqual(view.shape, (572, 1280, 3))
        np.testing.assert_array_equal(rgb, np.zeros_like(rgb))

    def test_unattended_run_must_be_bounded(self):
        with self.assertRaises(SystemExit):
            parse_args(["--no-display"])
        self.assertEqual(parse_args(["--no-display", "--frames", "3"]).frames, 3)


if __name__ == "__main__":
    unittest.main()
