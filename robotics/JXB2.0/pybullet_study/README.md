# PyBullet Mechanical Arm Study Route

This folder is a beginner-friendly route for learning PyBullet by building a simple robot arm simulator step by step.

## Before Running

Install PyBullet in your Python environment:

```powershell
pip install pybullet
```

Run scripts from this folder:

```powershell
cd C:\Users\12547\Desktop\JXB2.0\pybullet_study
python 01_hello_world.py
```

## Study Route

### Stage 0: Check Environment

File: `00_check_install.py`

Goal:
- Confirm PyBullet is installed.
- Understand `p.connect(p.DIRECT)` and `p.connect(p.GUI)`.

Run:

```powershell
python 00_check_install.py
```

### Stage 1: World, Camera, Gravity

File: `01_hello_world.py`

Goal:
- Open the PyBullet GUI.
- Add ground, gravity, and a falling official R2D2 model.
- Learn the simulation loop: `stepSimulation()`.
- Use official PyBullet bundled assets: `plane.urdf` and `r2d2.urdf`.

Run:

```powershell
python 01_hello_world.py
```

### Stage 2: Build Shapes

File: `02_create_shapes.py`

Goal:
- Create boxes and cylinders with visual and collision shapes.
- Understand the difference between visual shape and collision shape.

Run:

```powershell
python 02_create_shapes.py
```

### Stage 3: Robot Arm With Sliders

File: `03_arm_sliders.py`

Goal:
- Build a simple 5-joint robot arm.
- Control each joint with PyBullet debug sliders.
- Understand degrees vs radians.

Run:

```powershell
python 03_arm_sliders.py
```

### Stage 4: Automatic Motion

File: `04_arm_auto_motion.py`

Goal:
- Move joints automatically with a sine wave.
- Understand `POSITION_CONTROL`.

Run:

```powershell
python 04_arm_auto_motion.py
```

### Stage 5: Inverse Kinematics

File: `05_inverse_kinematics.py`

Goal:
- Move the robot arm end effector to a target point.
- Learn the basic idea of IK: target position -> joint angles.

Run:

```powershell
python 05_inverse_kinematics.py
```

### Stage 6: Record And Playback

File: `06_record_playback.py`

Goal:
- Record slider-controlled joint poses.
- Play them back as a simple action sequence.
- This is close to "teaching" a robot arm.

Run:

```powershell
python 06_record_playback.py
```

## Suggested Learning Order

1. Run each script once without changing code.
2. Change one number at a time, such as gravity, link length, motor force, or target point.
3. Read `common_robot.py` after Stage 3.
4. When you understand Stage 6, you can connect the simulated joint angles to ESP32 UDP commands.
