import cv2
import mediapipe as mp
import serial
import serial.tools.list_ports
import threading
import numpy as np
import time
import tkinter as tk
from tkinter import ttk
import os
import sys


def resource_path(relative_path):
    try:
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.abspath(".")
    return os.path.join(base_path, relative_path)

os.environ["MEDIAPIPE_DISABLE_GPU"] = "1"

BAUD_RATE = 115200
CAM_INDEX = 0


finger_sensitivity = [
    [20, 160],  # 食指
    [20, 160],  # 中指
    [20, 160],  # 无名指
    [50, 110]   # 大拇指
]
finger_names = ["食指", "中指", "无名指", "大拇指"]

finger_ang = [90, 90, 90, 90]
cam_running = False
cam_thread = None
ser = None

# Mediapipe
mp_hands = mp.solutions.hands
mp_draw = mp.solutions.drawing_utils
hands = mp_hands.Hands(
    static_image_mode=False,
    max_num_hands=1,
    min_detection_confidence=0.6,
    min_tracking_confidence=0.6
)

# ========== 串口通用功能 ==========
# 自动扫描所有串口
def get_all_ports():
    ports = serial.tools.list_ports.comports()
    return [f"{port.device} - {port.description}" for port in ports]

# 连接串口
def init_serial(port_name):
    global ser
    try:
        if ser and ser.is_open:
            ser.close()
            time.sleep(0.5)
        # 提取纯串口号（比如 COM7）
        pure_port = port_name.split(" - ")[0]
        ser = serial.Serial(pure_port, BAUD_RATE, timeout=0.1)
        time.sleep(1)
        return True
    except Exception as e:
        return False, str(e)

# 关闭串口
def close_serial():
    global ser
    if ser and ser.is_open:
        ser.close()

# 发送指令
def send_cmd():
    if not ser or not ser.is_open:
        return
    try:
        cmd = f"ALL:{finger_ang[0]},{finger_ang[1]},{finger_ang[2]},{finger_ang[3]}\n"
        ser.write(cmd.encode())
    except:
        pass

# ========== 手势角度计算 ==========
def calc_angle(v1, v2):
    v1 = v1 / np.linalg.norm(v1)
    v2 = v2 / np.linalg.norm(v2)
    dot = np.dot(v1, v2)
    dot = max(min(dot, 1.0), -1.0)
    return np.degrees(np.arccos(dot))

def get_finger_angle(landmarks, idx_list):
    p0 = np.array([landmarks[idx_list[0]].x, landmarks[idx_list[0]].y])
    p1 = np.array([landmarks[idx_list[1]].x, landmarks[idx_list[1]].y])
    p2 = np.array([landmarks[idx_list[2]].x, landmarks[idx_list[2]].y])
    p3 = np.array([landmarks[idx_list[3]].x, landmarks[idx_list[3]].y])
    return calc_angle(p1-p0, p2-p1) + calc_angle(p2-p1, p3-p2)

def get_thumb_angle(landmarks):
    p0 = np.array([landmarks[0].x, landmarks[0].y])
    p1 = np.array([landmarks[1].x, landmarks[1].y])
    p2 = np.array([landmarks[2].x, landmarks[2].y])
    p3 = np.array([landmarks[4].x, landmarks[4].y])
    return calc_angle(p1-p0, p2-p1) + calc_angle(p2-p1, p3-p2)

# ========== 摄像头手势循环 ==========
def cam_loop():
    global cam_running, finger_ang
    cap = cv2.VideoCapture(CAM_INDEX)
    while cam_running:
        ret, frame = cap.read()
        if not ret:
            break
        frame = cv2.flip(frame, 1)
        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        res = hands.process(rgb)

        if res.multi_hand_landmarks:
            lm = res.multi_hand_landmarks[0].landmark
            f1 = get_finger_angle(lm, [5,6,7,8])
            f2 = get_finger_angle(lm, [9,10,11,12])
            f3 = get_finger_angle(lm, [13,14,15,16])
            f4 = get_thumb_angle(lm)
            finger_raw = [f1, f2, f3, f4]

            for i in range(4):
                mi, ma = finger_sensitivity[i]
                finger_ang[i] = int(np.interp(finger_raw[i], [mi, ma], [90, 180]))
            send_cmd()
            mp_draw.draw_landmarks(frame, res.multi_hand_landmarks[0], mp_hands.HAND_CONNECTIONS)

        cv2.imshow("AI手势模仿", frame)
        if cv2.waitKey(1) & 0xFF == ord("q"):
            break
    cap.release()
    cv2.destroyAllWindows()

def start_cam():
    global cam_running, cam_thread
    if cam_running:
        return
    cam_running = True
    cam_thread = threading.Thread(target=cam_loop, daemon=True)
    cam_thread.start()
    lab_cam.config(text="摄像头：开启")

def stop_cam():
    global cam_running
    cam_running = False
    lab_cam.config(text="摄像头：关闭")

# ========== UI 灵敏度调节 ==========
def update_sens(i, t, v):
    finger_sensitivity[i][t] = int(v)
    labels[i].config(text=f"{finger_names[i]}: 直{finger_sensitivity[i][0]} | 弯{finger_sensitivity[i][1]}")

def reset_sens():
    global finger_sensitivity
    finger_sensitivity = [[20,160],[20,160],[20,160],[50,110]]
    for i in range(4):
        s_min[i].set(finger_sensitivity[i][0])
        s_max[i].set(finger_sensitivity[i][1])
        labels[i].config(text=f"{finger_names[i]}: 直{finger_sensitivity[i][0]} | 弯{finger_sensitivity[i][1]}")

# ========== 主UI界面 ==========
root = tk.Tk()
root.title("仿生机械手控制器")
root.geometry("680x620")
root.configure(bg="#0c1520")

# 样式配置
style = ttk.Style()
style.theme_use("clam")
style.configure("TButton", background="#1a2332", foreground="#00ffe7", font=("微软雅黑",10))
style.configure("TLabel", background="#0c1520", foreground="#e2e8f0")
style.configure("TCombobox", fieldbackground="#1a2332", background="#1a2332", foreground="#ffffff")

# 标题
tk.Label(root,text="🤖 仿生机械手 控制中心", font=("微软雅黑",18,"bold"),
         bg="#0c1520",fg="#00ffe7").pack(pady=12)

# 串口选择区
serial_frame = tk.Frame(root,bg="#151d2c")
serial_frame.pack(fill=tk.X,padx=20,pady=5)
tk.Label(serial_frame,text="串口选择",bg="#151d2c",fg="#00ffe7",font=("微软雅黑",10,"bold")).grid(row=0,column=0,padx=10,pady=8)

# 串口下拉框
port_list = get_all_ports()
port_var = tk.StringVar()
port_combo = ttk.Combobox(serial_frame, textvariable=port_var, values=port_list, width=35, state="readonly")
if port_list:
    port_combo.current(0)
port_combo.grid(row=0,column=1,padx=5)

# 刷新/连接按钮
def refresh_ports():
    port_list = get_all_ports()
    port_combo['values'] = port_list
    if port_list:
        port_combo.current(0)

def connect_port():
    selected_port = port_var.get()
    if not selected_port:
        tk.messagebox.showwarning("警告", "请先选择串口！")
        return
    res = init_serial(selected_port)
    if res:
        lab_com.config(text=f"串口：{selected_port.split(' - ')[0]} 已连接 ✅")
        tk.messagebox.showinfo("成功", "串口连接成功！")
    else:
        lab_com.config(text="串口：未连接")
        tk.messagebox.showerror("错误", f"串口连接失败！")

ttk.Button(serial_frame,text="🔄 刷新",command=refresh_ports).grid(row=0,column=2,padx=5)
ttk.Button(serial_frame,text="🔗 连接",command=connect_port).grid(row=0,column=3,padx=5)

# 状态栏
status = tk.Frame(root,bg="#151d2c")
status.pack(fill=tk.X,padx=20,pady=5)
lab_com = ttk.Label(status,text="串口：未连接")
lab_wave = ttk.Label(status,text="波浪：关闭")
lab_cam = ttk.Label(status,text="摄像头：关闭")
lab_com.grid(row=0,column=0,padx=20,pady=8)
lab_wave.grid(row=0,column=1,padx=20)
lab_cam.grid(row=0,column=2,padx=20)

# 功能按钮区
bf = tk.Frame(root,bg="#0c1520")
bf.pack(pady=12)

def wave_on():
    if not ser or not ser.is_open:
        tk.messagebox.showwarning("警告", "请先连接串口！")
        return
    try:
        ser.write(b"WAVEON\n")
        lab_wave.config(text="波浪：开启")
    except:
        pass

def wave_off():
    if not ser or not ser.is_open:
        return
    try:
        ser.write(b"WAVEOFF\n")
        lab_wave.config(text="波浪：关闭")
    except:
        pass

def reset_all():
    global finger_ang
    if not ser or not ser.is_open:
        tk.messagebox.showwarning("警告", "请先连接串口！")
        return
    try:
        ser.write(b"RESET\n")
        for i in range(4):
            finger_ang[i] = 90
        send_cmd()
    except:
        pass

ttk.Button(bf,text="🌊 开启波浪",command=wave_on).grid(row=0,column=0,padx=5)
ttk.Button(bf,text="⏸ 关闭波浪",command=wave_off).grid(row=0,column=1,padx=5)
ttk.Button(bf,text="🔄 全部复位",command=reset_all).grid(row=0,column=2,padx=5)
ttk.Button(bf,text="📷 开启手势",command=start_cam).grid(row=1,column=0,pady=10)
ttk.Button(bf,text="❌ 关闭手势",command=stop_cam).grid(row=1,column=1,pady=10)

# 灵敏度调节面板
sf = tk.Frame(root,bg="#151d2c")
sf.pack(fill=tk.X,padx=20,pady=10)
tk.Label(sf,text="🎛️ 四指独立灵敏度调节",font=("微软雅黑",12,"bold"),
         bg="#151d2c",fg="#00ffe7").grid(row=0,column=0,columnspan=4,pady=8)

labels = []
s_min = []
s_max = []

for i in range(4):
    l = tk.Label(sf,text=f"{finger_names[i]}: 直{finger_sensitivity[i][0]} | 弯{finger_sensitivity[i][1]}",
                 bg="#151d2c",fg="#e2e8f0")
    l.grid(row=i*2+1,column=0,columnspan=4)
    labels.append(l)

    tk.Label(sf,text="伸直阈值",bg="#151d2c",fg="#a0b3c4").grid(row=i*2+2,column=0,padx=5)
    sm = tk.Scale(sf,from_=10,to=80,orient=tk.HORIZONTAL,bg="#1a2332",fg="#00ffe7",length=180,
                  command=lambda v,i=i: update_sens(i,0,v))
    sm.set(finger_sensitivity[i][0])
    sm.grid(row=i*2+2,column=1,padx=5)
    s_min.append(sm)

    tk.Label(sf,text="弯曲阈值",bg="#151d2c",fg="#a0b3c4").grid(row=i*2+2,column=2,padx=5)
    sx = tk.Scale(sf,from_=90,to=180,orient=tk.HORIZONTAL,bg="#1a2332",fg="#00ffe7",length=180,
                  command=lambda v,i=i: update_sens(i,1,v))
    sx.set(finger_sensitivity[i][1])
    sx.grid(row=i*2+2,column=3,padx=5)
    s_max.append(sx)

ttk.Button(sf,text="🔙 恢复默认灵敏度",command=reset_sens).grid(row=9,column=0,columnspan=4,pady=10)

# 键盘控制
def key(e):
    s = 10
    c = e.char
    if c=="1": finger_ang[0]=min(180,finger_ang[0]+s)
    if c=="2": finger_ang[0]=max(90,finger_ang[0]-s)
    if c=="3": finger_ang[1]=min(180,finger_ang[1]+s)
    if c=="4": finger_ang[1]=max(90,finger_ang[1]-s)
    if c=="5": finger_ang[2]=min(180,finger_ang[2]+s)
    if c=="6": finger_ang[2]=max(90,finger_ang[2]-s)
    if c=="7": finger_ang[3]=min(180,finger_ang[3]+s)
    if c=="8": finger_ang[3]=max(90,finger_ang[3]-s)
    send_cmd()
root.bind("<Key>", key)

# 底部提示
tk.Label(root,text="键盘1-8控制四指弯曲 | 弯曲阈值越小，灵敏度越高",fg="#64748b",bg="#0c1520").pack(pady=3)
tk.Label(root,text="使用步骤：选择串口 → 点击连接 → 开启波浪/手势",fg="#64748b",bg="#0c1520").pack(pady=2)

# 窗口关闭释放资源
def on_close():
    global cam_running, ser
    cam_running = False
    close_serial()
    root.destroy()

root.protocol("WM_DELETE_WINDOW", on_close)
root.mainloop()