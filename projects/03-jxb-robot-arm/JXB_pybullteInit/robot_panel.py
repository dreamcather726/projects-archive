import runpy
from pathlib import Path


PANEL_PATH = Path(__file__).resolve().parent / "pybullet" / "robot_panel.py"
runpy.run_path(str(PANEL_PATH), run_name="__main__")
