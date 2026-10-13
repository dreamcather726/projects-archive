import runpy
from pathlib import Path


TEST_PATH = Path(__file__).resolve().parent / "pybullet" / "udp_test.py"
runpy.run_path(str(TEST_PATH), run_name="__main__")
