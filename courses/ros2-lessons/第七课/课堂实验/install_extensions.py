"""Extend the lesson 6 package while preserving its original model and display launch."""
from pathlib import Path
import shutil
import sys
import xml.etree.ElementTree as ET

if len(sys.argv) != 2:
    raise SystemExit('Usage: python3 install_extensions.py /absolute/path/my_robot_description')
target = Path(sys.argv[1]).expanduser().resolve()
source = Path(__file__).resolve().parent
cmake = target / 'CMakeLists.txt'
package = target / 'package.xml'
if not (target / 'urdf/my_robot.xacro').is_file() or not cmake.is_file() or not package.is_file():
    raise SystemExit('Complete the lesson 6 model package first; no files changed.')
text = cmake.read_text(encoding='utf-8')
old = 'install(DIRECTORY urdf launch rviz DESTINATION'
new = 'install(DIRECTORY urdf launch rviz worlds DESTINATION'
if old not in text and new not in text:
    raise SystemExit('Unexpected CMake layout; inspect resource install rules before extending.')
tree = ET.parse(package)
root = tree.getroot()
for dependency in ['gazebo_ros', 'gazebo_plugins']:
    if not any(element.text == dependency for element in root.findall('exec_depend')):
        ET.SubElement(root, 'exec_depend').text = dependency
for filename in ['urdf/my_robot_sim.xacro', 'worlds/my_world.world', 'launch/gazebo.launch.py']:
    destination = target / filename
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source / filename, destination)
cmake.write_text(text.replace(old, new), encoding='utf-8')
tree.write(package, encoding='utf-8', xml_declaration=True)
print('Added simulation Xacro, world, launch, resource installation and runtime dependencies.')
