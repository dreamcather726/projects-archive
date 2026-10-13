#-*-coding:utf-8-*-
# date:2021-04-5
# Author: Eric.Lee
# function: Inference

import os
import torch
import torch.nn as nn
import numpy as np
import math
import cv2

from models.resnet import resnet50
from hand_data_iter.datasets import draw_bd_handpose

# Calculate finger bend degrees
# 稳定版：计算手指弯曲程度（滤波 + 抗抖动 + 防异常）
finger_history = {}  # 全局缓存，用于平滑滤波
ALPHA = 0.3          # 平滑系数：0.1~0.4 越小越稳

def calculate_finger_bend(keypoints):
    """
    稳定版手指弯曲度计算（抗抖动、平滑、鲁棒）
    输出 0~1，0=伸直，1=完全弯曲
    """
    fingers = {
        "拇指": [4, 3, 2],
        "食指": [8, 7, 6],
        "中指": [12, 11, 10],
        "无名指": [16, 15, 14],
        "小指": [20, 19, 18]
    }
    bend_degrees = {}

    for finger_name, indices in fingers.items():
        try:
            # 获取三个关节点
            p1 = np.array([keypoints[str(indices[0])]["x"], keypoints[str(indices[0])]["y"]])
            p2 = np.array([keypoints[str(indices[1])]["x"], keypoints[str(indices[1])]["y"]])
            p3 = np.array([keypoints[str(indices[2])]["x"], keypoints[str(indices[2])]["y"]])

            # 计算两个骨段向量
            v1 = p2 - p1
            v2 = p3 - p2

            # 安全过滤：向量太短说明关键点检测失败，直接跳过
            len1 = np.linalg.norm(v1)
            len2 = np.linalg.norm(v2)
            if len1 < 3 or len2 < 3:
                current_degree = finger_history.get(finger_name, 0.0)
                bend_degrees[finger_name] = round(current_degree, 2)
                continue

            # 计算夹角
            cos_theta = np.dot(v1, v2) / (len1 * len2)
            cos_theta = np.clip(cos_theta, -0.99, 0.99)  # 防极值抖动
            angle = np.arccos(cos_theta) * 180 / np.pi

            # 角度映射到弯曲度 0~1
            raw_degree = 1.0 - (angle / 180.0)
            raw_degree = np.clip(raw_degree, 0.0, 1.0)

            # ---------------- 核心：低通滤波（平滑） ----------------
            if finger_name in finger_history:
                smooth_degree = ALPHA * raw_degree + (1 - ALPHA) * finger_history[finger_name]
            else:
                smooth_degree = raw_degree

            # 保存历史值
            finger_history[finger_name] = smooth_degree
            bend_degrees[finger_name] = round(smooth_degree, 2)

        except:
            # 异常保护
            bend_degrees[finger_name] = round(finger_history.get(finger_name, 0.0), 2)

    return bend_degrees
# Get English finger names
def get_english_finger_names():
    """
    Get English finger names mapping
    """
    return {
        "拇指": "Thumb",
        "食指": "Index",
        "中指": "Middle",
        "无名指": "Ring",
        "小指": "Pinky"
    }

# Draw finger bend degrees on image
def draw_finger_bend(img, bend_degrees, position=(10, 30)):
    """
    Draw finger bend degrees on image (English)
    """
    font = cv2.FONT_HERSHEY_SIMPLEX
    font_scale = 0.6
    font_thickness = 2
    color = (0, 255, 0)
    line_height = 25
    
    # Get English finger names
    english_names = get_english_finger_names()
    
    y = position[1]
    for finger, degree in bend_degrees.items():
        # Use English name for display
        english_name = english_names.get(finger, finger)
        text = f"{english_name}: {degree:.2f}"
        cv2.putText(img, text, (position[0], y), font, font_scale, color, font_thickness)
        y += line_height

if __name__ == "__main__":

    # 使用默认参数，不再需要命令行参数
    model_path = './resnet_50-size-256-loss-0.0642.pth'
    model = 'resnet_50'
    num_classes = 42
    GPUS = '0'
    test_path = './image/'
    img_size = (256, 256)
    vis = True
    use_camera = True

    # 打印参数信息
    print('\n/******************* Project Hand Pose Inference ******************/\n')
    print('----------------------------------')
    print('model_path : {}'.format(model_path))
    print('model : {}'.format(model))
    print('num_classes : {}'.format(num_classes))
    print('GPUS : {}'.format(GPUS))
    print('test_path : {}'.format(test_path))
    print('img_size : {}'.format(img_size))
    print('vis : {}'.format(vis))
    print('use_camera : {}'.format(use_camera))
    print('----------------------------------')

    #---------------------------------------------------------------------------
    os.environ['CUDA_VISIBLE_DEVICES'] = GPUS

    test_path = test_path # 测试图片文件夹路径

    #---------------------------------------------------------------- 构建模型
    print('use model : %s'%(model))

    # 只保留resnet_50模型，因为我们使用的是resnet_50模型
    model_ = resnet50(num_classes = num_classes,img_size=img_size[0])

    use_cuda = torch.cuda.is_available()

    device = torch.device("cuda:0" if use_cuda else "cpu")
    model_ = model_.to(device)
    model_.eval() # 设置为前向推断模式

    # print(model_)# 打印模型结构

    # 加载测试模型
    if os.access(model_path,os.F_OK):# checkpoint
        chkpt = torch.load(model_path, map_location=device)
        model_.load_state_dict(chkpt)
        print('load test model : {}'.format(model_path))

    #---------------------------------------------------------------- 预测图片
    '''建议 检测手bbox后，crop手图片的预处理方式：
    # img 为原图
    x_min,y_min,x_max,y_max,score = bbox
    w_ = max(abs(x_max-x_min),abs(y_max-y_min))

    w_ = w_*1.1

    x_mid = (x_max+x_min)/2
    y_mid = (y_max+y_min)/2

    x1,y1,x2,y2 = int(x_mid-w_/2),int(y_mid-w_/2),int(x_mid+w_/2),int(y_mid+w_/2)

    x1 = np.clip(x1,0,img.shape[1]-1)
    x2 = np.clip(x2,0,img.shape[1]-1)

    y1 = np.clip(y1,0,img.shape[0]-1)
    y2 = np.clip(y2,0,img.shape[0]-1)
    '''
    with torch.no_grad():
        if use_camera:
            # 使用摄像头
            cap = cv2.VideoCapture(0)
            if not cap.isOpened():
                print("Cannot open camera")
                exit()
            
            print("Using camera for real-time hand pose detection, press ESC to exit")
            
            while True:
                ret, img = cap.read()
                if not ret:
                    print("Cannot read camera frame")
                    break
                
                img_width = img.shape[1]
                img_height = img.shape[0]
                # 输入图片预处理
                img_ = cv2.resize(img, (img_size[1],img_size[0]), interpolation = cv2.INTER_CUBIC)
                img_ = img_.astype(np.float32)
                img_ = (img_-128.)/256.

                img_ = img_.transpose(2, 0, 1)
                img_ = torch.from_numpy(img_)
                img_ = img_.unsqueeze_(0)

                if use_cuda:
                    img_ = img_.cuda()  # (bs, 3, h, w)
                pre_ = model_(img_.float()) # 模型推理
                output = pre_.cpu().detach().numpy()
                output = np.squeeze(output)

                pts_hand = {} #构建关键点连线可视化结构
                for i in range(int(output.shape[0]/2)):
                    x = (output[i*2+0]*float(img_width))
                    y = (output[i*2+1]*float(img_height))

                    pts_hand[str(i)] = {}
                    pts_hand[str(i)] = {
                        "x":x,
                        "y":y,
                        }
                draw_bd_handpose(img,pts_hand,0,0) # 绘制关键点连线

                #------------- 绘制关键点
                for i in range(int(output.shape[0]/2)):
                    x = (output[i*2+0]*float(img_width))
                    y = (output[i*2+1]*float(img_height))

                    cv2.circle(img, (int(x),int(y)), 3, (255,50,60),-1)
                    cv2.circle(img, (int(x),int(y)), 1, (255,150,180),-1)

                # 计算手指弯曲程度
                bend_degrees = calculate_finger_bend(pts_hand)
                
                # 在控制台打印中文的手指弯曲程度
                print("\r手指弯曲程度:", end="")
                for finger, degree in bend_degrees.items():
                    print(f"{finger}: {degree:.2f}", end=" ")
                
                # 在窗口中显示英文的手指弯曲程度
                draw_finger_bend(img, bend_degrees)

                if vis:
                    cv2.imshow('Camera Hand Pose', img)
                    if cv2.waitKey(1) == 27:  # ESC键退出
                        break
            
            cap.release()

    cv2.destroyAllWindows()

    print('well done ')
