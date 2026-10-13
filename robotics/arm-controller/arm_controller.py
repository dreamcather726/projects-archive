#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
机械臂控制上位机
用于控制 ESP32 控制的 6 个舵机
通信协议：
- 单独控制：SERVO1:角度
- 同时控制：ALL:角度1,角度2,角度3,角度4,角度5,角度6
- 角度范围：0-180
"""

import tkinter as tk
from tkinter import ttk, messagebox
import serial
import serial.tools.list_ports
import threading
import time

class ArmController:
    def __init__(self, root):
        self.root = root
        self.root.title("机械臂控制器")
        self.root.geometry("600x500")
        self.root.resizable(False, False)
        
        # 串口相关
        self.serial_port = None
        self.baud_rate = 115200
        self.is_connected = False
        
        # 舵机角度值
        self.servo_angles = [90, 90, 90, 90, 90, 90]
        
        # 创建主界面
        self.create_widgets()
        
        # 自动扫描可用串口
        self.scan_ports()
        
    def create_widgets(self):
        # 创建主框架
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # 串口控制区域
        port_frame = ttk.LabelFrame(main_frame, text="串口设置", padding="10")
        port_frame.pack(fill=tk.X, pady=5)
        
        # 串口选择
        ttk.Label(port_frame, text="串口：").grid(row=0, column=0, sticky=tk.W, pady=5)
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(port_frame, textvariable=self.port_var, width=20)
        self.port_combo.grid(row=0, column=1, sticky=tk.W, pady=5)
        
        # 扫描按钮
        ttk.Button(port_frame, text="扫描串口", command=self.scan_ports).grid(row=0, column=2, padx=10, pady=5)
        
        # 连接/断开按钮
        self.connect_btn = ttk.Button(port_frame, text="连接", command=self.toggle_connection)
        self.connect_btn.grid(row=0, column=3, padx=10, pady=5)
        
        # 连接状态
        self.status_var = tk.StringVar(value="未连接")
        ttk.Label(port_frame, textvariable=self.status_var).grid(row=0, column=4, sticky=tk.W, pady=5)
        
        # 舵机控制区域
        servo_frame = ttk.LabelFrame(main_frame, text="舵机控制", padding="10")
        servo_frame.pack(fill=tk.BOTH, expand=True, pady=5)
        
        # 舵机滑块
        self.sliders = []
        self.slider_vars = []
        
        for i in range(6):
            frame = ttk.Frame(servo_frame)
            frame.pack(fill=tk.X, pady=5)
            
            ttk.Label(frame, text=f"舵机{i+1}：").pack(side=tk.LEFT, padx=5)
            
            var = tk.IntVar(value=self.servo_angles[i])
            self.slider_vars.append(var)
            
            slider = ttk.Scale(frame, from_=0, to=180, orient=tk.HORIZONTAL, length=300, variable=var, command=lambda v, idx=i: self.on_slider_change(idx, v))
            slider.pack(side=tk.LEFT, padx=5)
            self.sliders.append(slider)
            
            spinbox = ttk.Spinbox(frame, from_=0, to=180, width=5, textvariable=var, command=lambda idx=i: self.on_spinbox_change(idx))
            spinbox.pack(side=tk.LEFT, padx=5)
        
        # 控制按钮区域
        button_frame = ttk.Frame(main_frame, padding="10")
        button_frame.pack(fill=tk.X, pady=5)
        
        # 复位按钮
        ttk.Button(button_frame, text="复位所有舵机", command=self.reset_all_servos).pack(side=tk.LEFT, padx=10)
        
        # 同时控制按钮
        ttk.Button(button_frame, text="同时控制", command=self.control_all_servos).pack(side=tk.LEFT, padx=10)
        

        
    def scan_ports(self):
        """扫描可用的串口"""
        ports = serial.tools.list_ports.comports()
        port_list = [port.device for port in ports]
        self.port_combo['values'] = port_list
        if port_list:
            self.port_var.set(port_list[0])
    
    def toggle_connection(self):
        """切换串口连接状态"""
        if not self.is_connected:
            # 尝试连接
            port = self.port_var.get()
            if not port:
                messagebox.showerror("错误", "请选择串口")
                return
            
            try:
                self.serial_port = serial.Serial(port, self.baud_rate, timeout=0.1)
                self.is_connected = True
                self.connect_btn.config(text="断开")
                self.status_var.set("已连接")
                
                # 启动接收线程
                self.receive_thread = threading.Thread(target=self.receive_data, daemon=True)
                self.receive_thread.start()
                
                messagebox.showinfo("成功", f"已连接到 {port}")
            except Exception as e:
                messagebox.showerror("错误", f"连接失败：{str(e)}")
        else:
            # 断开连接
            if self.serial_port:
                self.serial_port.close()
            self.is_connected = False
            self.connect_btn.config(text="连接")
            self.status_var.set("未连接")
            messagebox.showinfo("成功", "已断开连接")
    
    def receive_data(self):
        """接收串口数据"""
        while self.is_connected:
            try:
                if self.serial_port and self.serial_port.in_waiting:
                    # 读取数据但不显示
                    self.serial_port.readline()
                time.sleep(0.1)
            except Exception as e:
                pass
    

    
    def on_slider_change(self, idx, value):
        """滑块值变化时的处理"""
        angle = int(float(value))
        self.servo_angles[idx] = angle
        # 实时发送串口数据
        self.control_single_servo(idx + 1, angle)
    
    def on_spinbox_change(self, idx):
        """数字框值变化时的处理"""
        angle = self.slider_vars[idx].get()
        self.servo_angles[idx] = angle
        self.sliders[idx].set(angle)
        # 实时发送串口数据
        self.control_single_servo(idx + 1, angle)
    

    
    def reset_all_servos(self):
        """复位所有舵机到90度"""
        for i in range(6):
            self.servo_angles[i] = 90
            self.slider_vars[i].set(90)
            self.sliders[i].set(90)
        
        self.control_all_servos()
    
    def control_all_servos(self):
        """同时控制所有舵机"""
        if not self.is_connected:
            messagebox.showerror("错误", "请先连接串口")
            return
        
        angles = ','.join(map(str, self.servo_angles))
        command = f"ALL:{angles}"
        
        try:
            self.serial_port.write((command + '\n').encode('utf-8'))
        except Exception as e:
            messagebox.showerror("错误", f"发送失败：{str(e)}")
    
    def control_single_servo(self, servo_num, angle):
        """控制单个舵机"""
        if not self.is_connected:
            return
        
        command = f"SERVO{servo_num}:{angle}"
        
        try:
            self.serial_port.write((command + '\n').encode('utf-8'))
        except Exception as e:
            pass

if __name__ == "__main__":
    root = tk.Tk()
    app = ArmController(root)
    root.mainloop()
