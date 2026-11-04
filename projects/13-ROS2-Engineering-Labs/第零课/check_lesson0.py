from pathlib import Path
import ast
import concurrent.futures
import re
import urllib.request
import sys

root = Path(__file__).resolve().parent
doc = root / 'ROS2_入门介绍_教师讲义_严格PPT顺序版.md'
content = doc.read_text(encoding='utf-8')
pages = [int(n) for n in re.findall(r'^## 第 (\d+) 页｜', content, re.M)]
assert pages == list(range(1, 24)), pages
assert len(re.findall(r'^```', content, re.M)) % 2 == 0
blocks = re.findall(r"<<'PY'\n(.*?)\nPY", content, re.S)
assert len(blocks) == 2
for block in blocks:
    ast.parse(block)
print('PASS: 23 consecutive pages; code fences balanced; 2 Python blocks parsed')
print('Commands:', len(re.findall(r'^\s*(?:ros2 |colcon |source |rosdep )', content, re.M)))
assert '--wait-matching-subscriptions 0 --keep-alive 60' in content
if '--offline' in sys.argv:
    print('PASS: publish-before-subscribe condition explicitly set')
    sys.exit(0)

sources = {
    'topic_pub': 'https://raw.githubusercontent.com/ros2/ros2cli/humble/ros2topic/ros2topic/verb/pub.py',
    'qos_profiles': 'https://raw.githubusercontent.com/ros2/rmw/humble/rmw/include/rmw/qos_profiles.h',
    'qos_docs': 'https://raw.githubusercontent.com/ros2/ros2_documentation/humble/source/Concepts/Intermediate/About-Quality-of-Service-Settings.rst',
    'py_tutorial': 'https://raw.githubusercontent.com/ros2/ros2_documentation/humble/source/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Py-Publisher-And-Subscriber.rst',
    'nodes_tutorial': 'https://raw.githubusercontent.com/ros2/ros2_documentation/humble/source/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Nodes/Understanding-ROS2-Nodes.rst',
    'topics_tutorial': 'https://raw.githubusercontent.com/ros2/ros2_documentation/humble/source/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Topics/Understanding-ROS2-Topics.rst',
    'rep2000': 'https://raw.githubusercontent.com/ros-infrastructure/rep/master/rep-2000.rst',
}

def fetch(item):
    name, url = item
    try:
        with urllib.request.urlopen(url, timeout=20) as response:
            text = response.read().decode('utf-8')
        folder = root / '核对资料'
        folder.mkdir(exist_ok=True)
        (folder / (name + '.txt')).write_text(text, encoding='utf-8')
        return name, text
    except Exception as error:
        return name, 'FETCH FAILED: ' + str(error)

with concurrent.futures.ThreadPoolExecutor(max_workers=5) as pool:
    for name, text in pool.map(fetch, sources.items()):
        print(name, len(text), text[:100].replace('\n', ' '))
        if name == 'topic_pub':
            lines = text.splitlines()
            for i, line in enumerate(lines):
                if any(token in line for token in ['keep-alive', "'--times'", 'keep_alive', 'times !=', 'times ==']):
                    print('\n'.join(lines[max(0, i-2):i+5]))
        if name == 'qos_profiles':
            marker = text.find('rmw_qos_profile_parameters')
            print(text[marker:marker+500])
