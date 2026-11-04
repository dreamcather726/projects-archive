import math
import time

import pybullet as p
import pybullet_data


JOINT_COUNT = 5
LINK_LENGTHS = [0.45, 0.38, 0.32, 0.24, 0.18]


def create_robot_arm():
    """Create a simple 5-joint teaching robot arm directly with PyBullet shapes."""
    visual_shapes = []
    collision_shapes = []
    link_masses = []
    link_positions = []
    link_orientations = []
    inertial_positions = []
    inertial_orientations = []
    parent_indices = []
    joint_types = []
    joint_axes = []

    for index, length in enumerate(LINK_LENGTHS):
        visual_shapes.append(
            p.createVisualShape(
                p.GEOM_BOX,
                halfExtents=[length / 2, 0.035, 0.035],
                rgbaColor=[0.2, 0.85 - index * 0.08, 1.0, 1.0],
            )
        )
        collision_shapes.append(
            p.createCollisionShape(
                p.GEOM_BOX,
                halfExtents=[length / 2, 0.035, 0.035],
            )
        )

        link_masses.append(0.2)
        link_positions.append([length / 2, 0, 0])
        link_orientations.append([0, 0, 0, 1])
        inertial_positions.append([0, 0, 0])
        inertial_orientations.append([0, 0, 0, 1])
        parent_indices.append(index)
        joint_types.append(p.JOINT_REVOLUTE)

        # Alternate axes so the arm has visible 3D motion.
        joint_axes.append([0, 0, 1] if index in (0, 3) else [0, 1, 0])

    robot_id = p.createMultiBody(
        baseMass=0,
        baseCollisionShapeIndex=p.createCollisionShape(p.GEOM_CYLINDER, radius=0.11, height=0.1),
        baseVisualShapeIndex=p.createVisualShape(
            p.GEOM_CYLINDER,
            radius=0.11,
            length=0.1,
            rgbaColor=[0.9, 0.9, 0.95, 1],
        ),
        basePosition=[0, 0, 0.08],
        linkMasses=link_masses,
        linkCollisionShapeIndices=collision_shapes,
        linkVisualShapeIndices=visual_shapes,
        linkPositions=link_positions,
        linkOrientations=link_orientations,
        linkInertialFramePositions=inertial_positions,
        linkInertialFrameOrientations=inertial_orientations,
        linkParentIndices=parent_indices,
        linkJointTypes=joint_types,
        linkJointAxis=joint_axes,
    )

    joint_count = p.getNumJoints(robot_id)
    print(f"Created robot with {joint_count} joints")

    for joint in range(joint_count):
        p.setJointMotorControl2(robot_id, joint, p.VELOCITY_CONTROL, force=0)

    return robot_id


def add_debug_sliders(joint_count):
    """PyBullet GUI sliders return degrees; motors need radians."""
    sliders = []
    defaults = [90, 0, 180, 130, 90]
    for index, value in enumerate(defaults[:joint_count]):
        sliders.append(p.addUserDebugParameter(f"J{index + 1} angle", 0, 180, value))
    return sliders


def set_camera():
    p.resetDebugVisualizerCamera(
        cameraDistance=2.1,
        cameraYaw=45,
        cameraPitch=-32,
        cameraTargetPosition=[0.45, 0, 0.35],
    )


def main():
    # GUI mode opens the PyBullet 3D window. Use p.DIRECT when you only need simulation.
    p.connect(p.GUI)
    p.setAdditionalSearchPath(pybullet_data.getDataPath())
    p.setGravity(0, 0, -9.8)
    p.loadURDF("plane.urdf")
    set_camera()

    robot_id = create_robot_arm()
    sliders = add_debug_sliders(p.getNumJoints(robot_id))

    while p.isConnected():
        for joint, slider in enumerate(sliders):
            angle_deg = p.readUserDebugParameter(slider)
            angle_rad = math.radians(angle_deg - 90)
            p.setJointMotorControl2(
                robot_id,
                joint,
                p.POSITION_CONTROL,
                targetPosition=angle_rad,
                force=2.0,
                positionGain=0.08,
            )

        p.stepSimulation()
        time.sleep(1 / 240)


if __name__ == "__main__":
    main()
