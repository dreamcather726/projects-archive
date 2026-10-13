# -*- coding: utf-8 -*-
"""
舵机校准模块
参考LeRobot实现的校准方法
使用set_half_turn_homings和record_ranges_of_motion方法
"""

import json
import time
import sys
from pathlib import Path
from typing import Dict, Optional, Tuple
from .motor_bus import MotorBus

sys.path.append(str(Path(__file__).parent.parent))
from config.motor_config import MOTOR_IDS, MOTOR_RESOLUTION, OperatingMode


def enter_pressed() -> bool:
    """
    检查用户是否按下回车键（非阻塞）
    
    Returns:
        bool: 如果按下回车返回True
    """
    import select
    import sys
    
    # Windows系统使用msvcrt
    if sys.platform == 'win32':
        import msvcrt
        if msvcrt.kbhit():
            key = msvcrt.getch()
            return key in [b'\r', b'\n']
        return False
    else:
        # Unix系统使用select
        return select.select([sys.stdin], [], [], 0)[0]


class MotorCalibration:
    """
    舵机校准类（参考LeRobot实现）
    负责舵机的零点校准和运动范围记录
    """
    
    def __init__(self, motor_bus: MotorBus, calibration_dir: str = "./calibration"):
        """
        初始化校准工具
        
        Args:
            motor_bus: 已连接的舵机总线
            calibration_dir: 校准数据保存目录
        """
        self.bus = motor_bus
        self.calibration_dir = Path(calibration_dir)
        self.calibration_dir.mkdir(parents=True, exist_ok=True)
        
        self.calibration_data = {}
    
    def disable_all_torque(self):
        """关闭所有舵机的扭矩"""
        print("\n关闭所有舵机扭矩...")
        for motor_name, motor_id in MOTOR_IDS.items():
            if self.bus.disable_torque(motor_id):
                print(f"  ✓ {motor_name} (ID={motor_id}) 扭矩已关闭")
            else:
                print(f"  ✗ {motor_name} (ID={motor_id}) 扭矩关闭失败")
    
    def enable_all_torque(self):
        """使能所有舵机的扭矩"""
        print("\n使能所有舵机扭矩...")
        for motor_name, motor_id in MOTOR_IDS.items():
            if self.bus.enable_torque(motor_id):
                print(f"  ✓ {motor_name} (ID={motor_id}) 扭矩已使能")
            else:
                print(f"  ✗ {motor_name} (ID={motor_id}) 扭矩使能失败")
    
    def set_all_position_mode(self):
        """设置所有舵机为位置控制模式"""
        print("\n设置所有舵机为位置控制模式...")
        for motor_name, motor_id in MOTOR_IDS.items():
            if self.bus.set_operating_mode(motor_id, OperatingMode.POSITION):
                print(f"  ✓ {motor_name} (ID={motor_id})")
            else:
                print(f"  ✗ {motor_name} (ID={motor_id}) 设置失败")
        time.sleep(0.5)
    
    def read_all_positions(self) -> Dict[str, int]:
        """
        读取所有舵机的当前位置
        
        Returns:
            Dict[str, int]: {舵机名称: 位置值}
        """
        positions = {}
        for motor_name, motor_id in MOTOR_IDS.items():
            pos = self.bus.get_present_position(motor_id)
            if pos is not None:
                positions[motor_name] = pos
        return positions
    
    def set_half_turn_homings(self, motors: list[str] = None) -> Dict[str, int]:
        """
        将每个舵机的当前位置设置为半圈位置（参考LeRobot实现）
        
        原理（Feetech舵机）：
        Present_Position = Actual_Position - Homing_Offset
        
        我们希望Present_Position = 2047 (12位编码器的半圈)
        因此：Homing_Offset = Actual_Position - 2047
        
        Args:
            motors: 要校准的舵机名称列表，None表示所有舵机（包括wrist_roll）
            
        Returns:
            Dict[str, int]: {舵机名称: 零点偏移值}
        """
        if motors is None:
            # 包括所有舵机，包括wrist_roll
            motors = list(MOTOR_IDS.keys())
        
        print(f"\n正在读取当前位置（共{len(motors)}个舵机）...")
        actual_positions = {}
        for motor_name in motors:
            motor_id = MOTOR_IDS[motor_name]
            pos = self.bus.get_present_position(motor_id)
            if pos is not None:
                actual_positions[motor_name] = pos
                print(f"  {motor_name}: {pos}")
        
        # 计算零点偏移
        # 对于4096分辨率的舵机，半圈位置是2047
        half_turn = (MOTOR_RESOLUTION - 1) // 2  # 2047
        
        homing_offsets = {}
        print(f"\n计算零点偏移（基准值={half_turn}）...")
        for motor_name, pos in actual_positions.items():
            offset = pos - half_turn
            homing_offsets[motor_name] = offset
            print(f"  {motor_name}: 偏移 = {pos} - {half_turn} = {offset}")
        
        # 写入偏移值到舵机
        print(f"\n写入零点偏移到舵机（共{len(homing_offsets)}个）...")
        for motor_name, offset in homing_offsets.items():
            motor_id = MOTOR_IDS[motor_name]
            # 需要先关闭扭矩
            self.bus.disable_torque(motor_id)
            time.sleep(0.05)
            
            success = self.bus.write_control_table(motor_id, "Homing_Offset", offset)
            if success:
                print(f"  ✓ {motor_name} (ID={motor_id}) 零点偏移已写入: {offset}")
            else:
                print(f"  ✗ {motor_name} (ID={motor_id}) 零点偏移写入失败")
            time.sleep(0.05)
        
        print(f"\n✓ 已完成{len(homing_offsets)}个舵机的零点偏移设置")
        return homing_offsets
    
    def record_ranges_of_motion(self, motors: list[str] = None, 
                               display_values: bool = True) -> Tuple[Dict[str, int], Dict[str, int]]:
        """
        实时记录舵机运动范围（参考LeRobot实现）
        
        用户手动移动关节通过其整个运动范围，程序实时记录最小和最大位置
        按回车键停止记录
        
        Args:
            motors: 要记录的舵机名称列表，None表示所有舵机
            display_values: 是否显示实时位置表格
            
        Returns:
            Tuple[Dict[str, int], Dict[str, int]]: (最小位置字典, 最大位置字典)
        """
        if motors is None:
            # 默认记录所有舵机
            motors = list(MOTOR_IDS.keys())
        
        print(f"\n正在读取起始位置...")
        mins = {}
        maxes = {}
        
        # 初始化：读取当前位置作为起始min/max
        for motor_name in motors:
            motor_id = MOTOR_IDS[motor_name]
            pos = self.bus.get_present_position(motor_id)
            if pos is not None:
                mins[motor_name] = pos
                maxes[motor_name] = pos
                print(f"  {motor_name}: 起始位置 = {pos}")
            else:
                print(f"  {motor_name}: 读取失败，跳过")
        
        # 检查是否所有舵机都初始化成功
        if len(mins) != len(motors):
            print(f"\n警告：部分舵机初始化失败")
        
        print("\n开始记录运动范围...")
        print("请移动所有关节通过其完整的运动范围")
        print("按 ENTER 键停止记录\n")
        
        user_pressed_enter = False
        line_count = 0
        
        while not user_pressed_enter:
            # 读取所有舵机位置
            positions = {}
            for motor_name in motors:
                motor_id = MOTOR_IDS[motor_name]
                pos = self.bus.get_present_position(motor_id)
                if pos is not None:
                    positions[motor_name] = pos
                    
                    # 更新最小最大值（只有已初始化的舵机才更新）
                    if motor_name in mins:
                        mins[motor_name] = min(pos, mins[motor_name])
                        maxes[motor_name] = max(pos, maxes[motor_name])
                    else:
                        # 如果之前初始化失败，现在成功读取了，则初始化
                        mins[motor_name] = pos
                        maxes[motor_name] = pos
            
            if display_values:
                # 清除之前的输出
                if line_count > 0:
                    # 向上移动光标并清除
                    for _ in range(line_count):
                        print("\033[A\033[K", end="")
                
                # 显示表格
                print("-" * 65)
                print(f"{'NAME':<15} | {'MIN':>6} | {'POS':>6} | {'MAX':>6} | {'RANGE':>8}")
                print("-" * 65)
                line_count = 3
                
                for motor_name in motors:
                    if motor_name in positions:
                        range_val = maxes[motor_name] - mins[motor_name]
                        print(f"{motor_name:<15} | {mins[motor_name]:>6} | {positions[motor_name]:>6} | {maxes[motor_name]:>6} | {range_val:>8}")
                        line_count += 1
            
            # 检查是否按下回车
            if enter_pressed():
                user_pressed_enter = True
            else:
                time.sleep(0.05)  # 50ms刷新率
        
        print("\n" + "="*60)
        print("运动范围记录完成")
        print("="*60)
        
        # 检查是否有相同的min和max
        same_min_max = [m for m in motors if mins[m] == maxes[m]]
        if same_min_max:
            print(f"\n⚠️  警告：以下舵机未移动（min==max）: {same_min_max}")
        
        return mins, maxes
    
    def calibrate_all_motors(self) -> Dict:
        """
        完整校准流程（完全对齐LeRobot实现）
        
        步骤：
        1. 关闭扭矩，设置位置模式
        2. 移动到中间位置
        3. 设置零点偏移（所有舵机包括wrist_roll）
        4. 记录运动范围（除wrist_roll外）
        5. wrist_roll使用全范围
        
        Returns:
            Dict: 校准数据
        """
        print("\n" + "="*60)
        print("SO-ARM100 机械臂校准（LeRobot方式）")
        print("="*60)
        
        # 1. 关闭扭矩和设置位置模式
        self.disable_all_torque()
        self.set_all_position_mode()
        
        # 2. 移动到中间位置并设置零点偏移
        print("\n" + "="*60)
        input("将机械臂移动到运动范围的中间位置，然后按回车键...")
        print("="*60)
        
        # 3. 设置零点偏移（所有舵机包括wrist_roll）
        homing_offsets = self.set_half_turn_homings()
        
        # 4. 记录运动范围（所有关节）
        print("\n" + "="*60)
        print("移动所有关节通过其完整的运动范围")
        print("实时记录位置，按 ENTER 键停止...")
        print("="*60)
        
        # 所有舵机都需要记录实际运动范围
        all_motors = list(MOTOR_IDS.keys())
        range_mins, range_maxes = self.record_ranges_of_motion(all_motors)
        
        # 6. 组合校准数据
        calibration_data = {}
        for motor_name, motor_id in MOTOR_IDS.items():
            calibration_data[motor_name] = {
                "id": motor_id,
                "drive_mode": 0,
                "homing_offset": homing_offsets.get(motor_name, 0),
                "range_min": range_mins.get(motor_name, 0),
                "range_max": range_maxes.get(motor_name, MOTOR_RESOLUTION - 1),
            }
        
        self.calibration_data = calibration_data
        
        # 7. 写入校准数据到舵机
        self.write_calibration_to_motors()
        
        # 显示校准结果
        print("\n" + "="*60)
        print("校准完成！")
        print("="*60)
        self.display_calibration()
        
        return calibration_data
    
    def save_calibration(self, robot_id: str = "default"):
        """
        保存校准数据到文件（根据robot_id命名）
        
        Args:
            robot_id: 机械臂ID，用于区分不同机械臂的校准文件
        """
        filename = f"so_arm100_calibration_{robot_id}.json"
        filepath = self.calibration_dir / filename
        
        with open(filepath, 'w', encoding='utf-8') as f:
            json.dump(self.calibration_data, f, indent=2, ensure_ascii=False)
        
        print(f"\n✓ 校准数据已保存到: {filepath}")
    
    def load_calibration(self, filename: str = "so_arm100_calibration.json") -> Optional[Dict]:
        """
        从文件加载校准数据
        
        Args:
            filename: 文件名
            
        Returns:
            Optional[Dict]: 校准数据，如果文件不存在返回None
        """
        filepath = self.calibration_dir / filename
        
        if not filepath.exists():
            print(f"✗ 校准文件不存在: {filepath}")
            return None
        
        with open(filepath, 'r', encoding='utf-8') as f:
            self.calibration_data = json.load(f)
        
        print(f"✓ 校准数据已加载: {filepath}")
        return self.calibration_data
    
    def display_calibration(self):
        """显示当前的校准数据"""
        if not self.calibration_data:
            print("没有校准数据")
            return
        
        print("\n" + "="*60)
        print("校准数据摘要")
        print("="*60)
        
        for motor_name, data in self.calibration_data.items():
            print(f"\n{motor_name} (ID={data['id']}):")
            print(f"  零点偏移: {data['homing_offset']}")
            print(f"  运动范围: [{data['range_min']}, {data['range_max']}]")
    
    def write_calibration_to_motors(self):
        """
        将校准数据写入舵机（零点偏移）
        
        注意：这会修改舵机的EEPROM
        """
        if not self.calibration_data:
            print("✗ 没有校准数据")
            return
        
        print("\n正在写入校准数据到舵机...")
        for motor_name, data in self.calibration_data.items():
            motor_id = data['id']
            offset = data['homing_offset']
            
            # 关闭扭矩
            self.bus.disable_torque(motor_id)
            time.sleep(0.1)
            
            # 写入零点偏移
            success = self.bus.write_control_table(motor_id, "Homing_Offset", offset)
            
            if success:
                print(f"  ✓ {motor_name}: {offset}")
            else:
                print(f"  ✗ {motor_name}: 写入失败")
            
            time.sleep(0.1)
        
        print("✓ 校准数据已写入舵机")
    
    def verify_calibration(self):
        """验证校准数据"""
        print("\n" + "="*60)
        print("验证校准数据")
        print("="*60)
        
        if not self.calibration_data:
            print("✗ 没有校准数据")
            return
        
        print("\n读取当前舵机位置...")
        current_positions = self.read_all_positions()
        
        print("\n校准验证结果:")
        for motor_name, pos in current_positions.items():
            if motor_name in self.calibration_data:
                data = self.calibration_data[motor_name]
                range_min = data['range_min']
                range_max = data['range_max']
                offset = data['homing_offset']
                
                in_range = range_min <= pos <= range_max
                status = "✓" if in_range else "✗"
                
                print(f"\n{motor_name}:")
                print(f"  当前位置: {pos}")
                print(f"  零点偏移: {offset}")
                print(f"  运动范围: [{range_min}, {range_max}]")
                print(f"  {status} {'在范围内' if in_range else '超出范围'}")
