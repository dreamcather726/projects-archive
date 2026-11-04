#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
手部追踪控制器
使用摄像头识别手指动作并控制Arduino手指
"""

import cv2
import mediapipe as mp
import serial
import time
import threading

class HandTrackingController:
    def __init__(self):
        # 初始化MediaPipe手部检测
        self.mp_hands = mp.solutions.hands
        self.hands = self.mp_hands.Hands(
            static_image_mode=False,
            max_num_hands=1,
            min_detection_confidence=0.7,
            min_tracking_confidence=0.5
        )
        self.mp_drawing = mp.solutions.drawing_utils
        
        # 串口设置
        self.serial_port = None
        self.baudrate = 115200
        
        # 手指状态
        self.finger_states = [90, 90, 90, 90]  # 四个手指的角度
        
        # 运行标志
        self.running = False
        self.cap = None
        
    def connect_serial(self, port):
        """连接串口"""
        try:
            self.serial_port = serial.Serial(port, self.baudrate, timeout=0.1)
            print(f"已连接到串口: {port}")
            return True
        except Exception as e:
            print(f"串口连接失败: {e}")
            return False
    
    def calculate_finger_angle(self, hand_landmarks, finger_idx):
        """计算手指弯曲角度"""
        # 手指关键点索引
        finger_indices = {
            0: [4, 3, 2, 1, 0],  # 拇指
            1: [8, 7, 6, 5],      # 食指
            2: [12, 11, 10, 9],   # 中指
            3: [16, 15, 14, 13]   # 无名指
        }
        
        if finger_idx not in finger_indices:
            return 90
        
        indices = finger_indices[finger_idx]
        
        # 获取手指关键点
        points = []
        for idx in indices:
            landmark = hand_landmarks.landmark[idx]
            points.append((landmark.x, landmark.y))
        
        # 简单计算手指弯曲程度 (0-180度)
        # 基于手指最后两个关节的角度
        if len(points) >= 3:
            # 计算向量
            v1 = (points[1][0] - points[0][0], points[1][1] - points[0][1])
            v2 = (points[2][0] - points[1][0], points[2][1] - points[1][1])
            
            # 计算夹角
            dot = v1[0] * v2[0] + v1[1] * v2[1]
            mag1 = (v1[0]**2 + v1[1]**2)**0.5
            mag2 = (v2[0]**2 + v2[1]**2)**0.5
            
            if mag1 * mag2 > 0:
                cos_theta = max(-1, min(1, dot / (mag1 * mag2)))
                angle = int(abs(mag1 - mag2) * 180)  # 简化计算
                angle = max(90, min(180, angle))  # 限制在90-180度
                return angle
        
        return 90
    
    def update_fingers(self, hand_landmarks):
        """更新手指状态"""
        for i in range(4):  # 四个手指
            angle = self.calculate_finger_angle(hand_landmarks, i)
            self.finger_states[i] = angle
        
        # 发送命令
        self.send_finger_commands()
    
    def send_finger_commands(self):
        """发送手指控制命令"""
        if not self.serial_port or not self.serial_port.is_open:
            return
        
        # 使用ALL指令同时控制四个手指
        command = f"ALL:{self.finger_states[0]},{self.finger_states[1]},{self.finger_states[2]},{self.finger_states[3]}\n"
        
        try:
            self.serial_port.write(command.encode('utf-8'))
            print(f"发送命令: {command.strip()}")
        except Exception as e:
            print(f"发送失败: {e}")
    
    def run(self):
        """主运行函数"""
        self.running = True
        self.cap = cv2.VideoCapture(0)
        
        while self.running:
            ret, frame = self.cap.read()
            if not ret:
                break
            
            # 镜像翻转
            frame = cv2.flip(frame, 1)
            
            # 转换为RGB
            rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            
            # 检测手部
            results = self.hands.process(rgb_frame)
            
            # 绘制手部关键点
            if results.multi_hand_landmarks:
                for hand_landmarks in results.multi_hand_landmarks:
                    self.mp_drawing.draw_landmarks(
                        frame, hand_landmarks, self.mp_hands.HAND_CONNECTIONS
                    )
                    
                    # 更新手指状态
                    self.update_fingers(hand_landmarks)
            
            # 显示手指状态
            for i, angle in enumerate(self.finger_states):
                cv2.putText(
                    frame, f'手指{i+1}: {angle}°', 
                    (20, 30 + i * 25), 
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2
                )
            
            # 显示画面
            cv2.imshow('手部追踪', frame)
            
            # 按ESC退出
            if cv2.waitKey(1) == 27:
                break
        
        # 清理
        self.stop()
    
    def stop(self):
        """停止运行"""
        self.running = False
        
        if self.cap:
            self.cap.release()
        
        cv2.destroyAllWindows()
        
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
            print("串口已关闭")

if __name__ == '__main__':
    controller = HandTrackingController()
    
    # 连接串口
    port = input("请输入串口名称 (例如 COM3): ")
    controller.connect_serial(port)
    
    print("开始手部追踪...")
    print("按 ESC 键退出")
    
    controller.run()
