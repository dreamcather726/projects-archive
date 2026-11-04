#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
手部追踪简单版
仅识别手指弯曲状态，不控制舵机
使用MediaPipe Task API
"""

import cv2
import math
import mediapipe as mp
from mediapipe.tasks.python import vision
from mediapipe.tasks.python.core import BaseOptions

class HandTracker:
    def __init__(self):
        # 初始化手势识别器
        base_options = BaseOptions(model_asset_path='hand_landmarker.task')
        options = vision.HandLandmarkerOptions(
            base_options=base_options,
            num_hands=1,
            min_hand_detection_confidence=0.5,
            min_hand_presence_confidence=0.5,
            min_tracking_confidence=0.5
        )
        self.detector = vision.HandLandmarker.create_from_options(options)
        self.cap = None
        self.running = False
    
    def calculate_distance(self, p1, p2):
        """计算两点距离"""
        return math.sqrt((p1[0] - p2[0])**2 + (p1[1] - p2[1])**2)
    
    def is_finger_bent(self, landmarks, finger_tip_idx, finger_pip_idx, wrist_idx):
        """判断手指是否弯曲"""
        # 获取关键点坐标
        tip = (landmarks[finger_tip_idx].x, landmarks[finger_tip_idx].y)
        pip = (landmarks[finger_pip_idx].x, landmarks[finger_pip_idx].y)
        wrist = (landmarks[wrist_idx].x, landmarks[wrist_idx].y)
        
        # 计算指尖到手腕的距离和PIP到手腕的距离
        tip_to_wrist = self.calculate_distance(tip, wrist)
        pip_to_wrist = self.calculate_distance(pip, wrist)
        
        # 如果指尖到手腕的距离小于PIP到手腕的距离，手指弯曲
        return tip_to_wrist < pip_to_wrist * 1.2
    
    def get_finger_states(self, landmarks):
        """获取四个手指的状态 (食指、中指、无名指、小指)"""
        # 手指关键点索引 (MediaPipe)
        fingers = [
            (8, 6, 0),   # 食指: 指尖, PIP关节, 手腕
            (12, 10, 0), # 中指
            (16, 14, 0), # 无名指
            (20, 18, 0)  # 小指
        ]
        
        states = []
        for tip_idx, pip_idx, wrist_idx in fingers:
            bent = self.is_finger_bent(landmarks, tip_idx, pip_idx, wrist_idx)
            states.append("弯曲" if bent else "伸直")
        
        return states
    
    def draw_landmarks(self, frame, landmarks):
        """绘制手部关键点"""
        h, w, _ = frame.shape
        # 绘制关键点
        for landmark in landmarks:
            x = int(landmark.x * w)
            y = int(landmark.y * h)
            cv2.circle(frame, (x, y), 3, (0, 255, 0), -1)
        
        # 绘制连接线
        connections = [
            (0, 1), (1, 2), (2, 3), (3, 4),  # 拇指
            (0, 5), (5, 6), (6, 7), (7, 8),  # 食指
            (0, 9), (9, 10), (10, 11), (11, 12),  # 中指
            (0, 13), (13, 14), (14, 15), (15, 16),  # 无名指
            (0, 17), (17, 18), (18, 19), (19, 20),  # 小指
            (5, 9), (9, 13), (13, 17)  # 手掌
        ]
        for start_idx, end_idx in connections:
            if start_idx < len(landmarks) and end_idx < len(landmarks):
                x1 = int(landmarks[start_idx].x * w)
                y1 = int(landmarks[start_idx].y * h)
                x2 = int(landmarks[end_idx].x * w)
                y2 = int(landmarks[end_idx].y * h)
                cv2.line(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
    
    def run(self):
        """主运行函数"""
        self.running = True
        self.cap = cv2.VideoCapture(0)
        
        if not self.cap.isOpened():
            print("错误: 无法打开摄像头")
            return
        
        print("手部追踪已启动...")
        print("按 ESC 键退出")
        
        while self.running:
            ret, frame = self.cap.read()
            if not ret:
                break
            
            # 镜像翻转
            frame = cv2.flip(frame, 1)
            
            # 转换为RGB
            rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            
            # 创建MediaPipe图像
            mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)
            
            # 检测手部
            detection_result = self.detector.detect(mp_image)
            
            # 绘制手部关键点
            if detection_result.hand_landmarks:
                for hand_landmarks in detection_result.hand_landmarks:
                    self.draw_landmarks(frame, hand_landmarks)
                    
                    # 获取手指状态
                    finger_states = self.get_finger_states(hand_landmarks)
                    
                    # 显示手指状态
                    finger_names = ["食指", "中指", "无名指", "小指"]
                    for i, (name, state) in enumerate(zip(finger_names, finger_states)):
                        color = (0, 0, 255) if state == "弯曲" else (0, 255, 0)
                        cv2.putText(
                            frame, f'{name}: {state}', 
                            (10, 30 + i * 30), 
                            cv2.FONT_HERSHEY_SIMPLEX, 0.8, color, 2
                        )
            else:
                cv2.putText(
                    frame, "未检测到手部", 
                    (10, 30), 
                    cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2
                )
            
            # 显示画面
            cv2.imshow('手部追踪', frame)
            
            # 按ESC退出
            if cv2.waitKey(1) == 27:
                break
        
        self.stop()
    
    def stop(self):
        """停止运行"""
        self.running = False
        
        if self.cap:
            self.cap.release()
        
        cv2.destroyAllWindows()
        print("程序已停止")

if __name__ == '__main__':
    tracker = HandTracker()
    tracker.run()
