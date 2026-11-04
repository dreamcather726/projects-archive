"""
SO-ARM100 遥操作示例
双臂配置：主臂（Leader）手动拖动，从臂（Follower）实时跟随
"""

import sys
from pathlib import Path

# 添加项目根目录到Python路径
project_root = Path(__file__).parent.parent
sys.path.insert(0, str(project_root))

import time
import logging
from core.robot import SO100Robot

# 设置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


class TeleOperation:
    """遥操作控制类"""
    
    def __init__(self, 
                 leader_port: str,
                 follower_port: str,
                 frequency: float = 50.0):
        """
        初始化遥操作
        
        Args:
            leader_port: 主臂串口
            follower_port: 从臂串口
            frequency: 控制频率（Hz）
        """
        self.leader_port = leader_port
        self.follower_port = follower_port
        self.frequency = frequency
        self.dt = 1.0 / frequency
        
        # 创建机械臂实例（指定不同的robot_id）
        self.leader = SO100Robot(
            port=leader_port,
            robot_id="leader",  # 主臂使用leader校准文件
            max_relative_target=None,  # 主臂不限制
            disable_torque_on_disconnect=False
        )
        
        self.follower = SO100Robot(
            port=follower_port,
            robot_id="follower",  # 从臂使用follower校准文件
            max_relative_target=None,  # 遥操作时不限制移动距离
            disable_torque_on_disconnect=True
        )
        
        self._running = False
    
    def connect(self):
        """连接双臂"""
        logger.info("=" * 60)
        logger.info("连接主臂（Leader）...")
        logger.info("=" * 60)
        self.leader.connect()
        
        logger.info("\n" + "=" * 60)
        logger.info("连接从臂（Follower）...")
        logger.info("=" * 60)
        self.follower.connect()
        
        # 主臂关闭扭矩（允许手动拖动）
        logger.info("\n关闭主臂扭矩（允许手动拖动）...")
        self.leader.disable_torque_all()
        
        # 从臂启用扭矩（执行跟随）
        logger.info("启用从臂扭矩（执行跟随）...")
        self.follower.enable_torque_all()
        
        logger.info("\n✓ 双臂连接完成，准备开始遥操作")
    
    def disconnect(self):
        """断开双臂"""
        logger.info("\n断开双臂连接...")
        self.leader.disconnect()
        self.follower.disconnect()
        logger.info("✓ 双臂已断开")
    
    def run(self, duration: float = None):
        """
        运行遥操作循环
        
        Args:
            duration: 运行时长（秒），None表示无限循环
        """
        logger.info("\n" + "=" * 60)
        logger.info("开始遥操作")
        logger.info(f"控制频率: {self.frequency} Hz")
        logger.info("按 Ctrl+C 停止")
        logger.info("=" * 60)
        
        self._running = True
        start_time = time.time()
        iteration = 0
        
        try:
            while self._running:
                loop_start = time.perf_counter()
                
                # 读取主臂位置
                leader_obs = self.leader.get_observation()
                
                # 发送给从臂
                self.follower.send_action(leader_obs)
                
                iteration += 1
                
                # 定期显示状态
                if iteration % 50 == 0:
                    elapsed = time.time() - start_time
                    actual_freq = iteration / elapsed
                    logger.info(
                        f"遥操作运行中... "
                        f"迭代: {iteration}, "
                        f"实际频率: {actual_freq:.1f} Hz, "
                        f"耗时: {elapsed:.1f}s"
                    )
                
                # 检查运行时长
                if duration is not None and (time.time() - start_time) >= duration:
                    logger.info(f"达到指定时长 {duration}s，停止遥操作")
                    break
                
                # 频率控制
                elapsed = time.perf_counter() - loop_start
                sleep_time = self.dt - elapsed
                if sleep_time > 0:
                    time.sleep(sleep_time)
                elif iteration % 50 == 0:
                    logger.warning(
                        f"循环耗时 {elapsed*1000:.1f}ms 超过目标 {self.dt*1000:.1f}ms"
                    )
        
        except KeyboardInterrupt:
            logger.info("\n收到中断信号，停止遥操作")
        
        finally:
            self._running = False
            total_time = time.time() - start_time
            avg_freq = iteration / total_time if total_time > 0 else 0
            logger.info(f"\n遥操作统计:")
            logger.info(f"  总迭代次数: {iteration}")
            logger.info(f"  总耗时: {total_time:.2f}s")
            logger.info(f"  平均频率: {avg_freq:.1f} Hz")
    
    def __enter__(self):
        """上下文管理器支持"""
        self.connect()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """上下文管理器支持"""
        self.disconnect()


def main():
    """主函数"""
    print("\n" + "=" * 60)
    print("SO-ARM100 双臂遥操作")
    print("=" * 60)
    
    # 输入串口
    leader_port = input("\n请输入主臂（Leader）串口 (默认 COM22): ").strip() or "COM22"
    follower_port = input("请输入从臂（Follower）串口 (默认 COM24): ").strip() or "COM24"
    
    # 输入控制频率
    freq_input = input("请输入控制频率 Hz (默认 50): ").strip()
    frequency = float(freq_input) if freq_input else 50.0
    
    # 创建遥操作实例
    teleop = TeleOperation(
        leader_port=leader_port,
        follower_port=follower_port,
        frequency=frequency
    )
    
    try:
        # 使用上下文管理器自动连接/断开
        with teleop:
            # 运行遥操作
            teleop.run()
    
    except Exception as e:
        logger.error(f"遥操作出错: {e}", exc_info=True)
    
    finally:
        print("\n遥操作已结束")


if __name__ == "__main__":
    main()
