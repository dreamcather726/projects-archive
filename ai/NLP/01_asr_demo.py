import os
import time
import numpy as np
import subprocess
import tempfile

from spacemit_asr import ASRModel

# 初始化ASR模型
asr_model = ASRModel()

def convert_audio_to_wav(input_file, output_file=None):
    """
    使用ffmpeg将音频文件转换为WAV格式
    """
    if output_file is None:
        output_file = tempfile.mktemp(suffix='.wav')
    
    try:
        # 使用ffmpeg转换音频格式
        cmd = [
            'ffmpeg', '-i', input_file, 
            '-ar', '16000',  # 采样率16kHz
            '-ac', '1',      # 单声道
            '-y',            # 覆盖输出文件
            output_file
        ]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode == 0:
            return output_file
        else:
            print(f"ffmpeg转换失败: {result.stderr}")
            return None
    except FileNotFoundError:
        print("错误：未找到ffmpeg，请安装ffmpeg")
        print("安装命令：sudo apt-get install ffmpeg")
        return None
    except Exception as e:
        print(f"音频转换出错: {e}")
        return None

def process_audio_file(audio_file_path):
    """
    处理音频文件并转换为文本
    """
    if not os.path.exists(audio_file_path):
        print(f"错误：音频文件不存在 - {audio_file_path}")
        return None
    
    # 检查文件大小
    file_size = os.path.getsize(audio_file_path)
    if file_size == 0:
        print("错误：音频文件为空")
        return None
    
    print(f"文件大小: {file_size} bytes")
    
    # 检查文件格式
    supported_formats = ['.wav', '.mp3', '.flac', '.m4a', '.ogg']
    file_ext = os.path.splitext(audio_file_path)[1].lower()
    
    converted_file = None
    try:
        # 如果不是WAV格式，尝试转换
        if file_ext != '.wav':
            print(f"检测到 {file_ext} 格式，尝试转换为WAV...")
            converted_file = convert_audio_to_wav(audio_file_path)
            if converted_file:
                audio_file_path = converted_file
                print(f"转换成功: {converted_file}")
            else:
                print("格式转换失败，尝试直接处理原文件...")
        
        print(f"正在处理音频文件: {audio_file_path}")
        print("ASR转换中...")
        
        # 使用ASR模型进行语音识别
        text = asr_model.generate(audio_file_path)
        
        if text and text.strip():
            return text.strip()
        else:
            print("警告：未识别到有效文本内容")
            return None
            
    except Exception as e:
        print(f"ASR处理出错: {str(e)}")
        print(f"错误类型: {type(e).__name__}")
        return None
    finally:
        # 清理临时文件
        if converted_file and os.path.exists(converted_file):
            try:
                os.remove(converted_file)
            except:
                pass

if __name__ == '__main__':
    print("=== 音频文件转文本工具 ===")
    print("支持格式: WAV, MP3, FLAC, M4A, OGG")
    print("建议使用16kHz采样率的WAV文件以获得最佳效果")
    
    try:
        while True:
            # 获取用户输入的音频文件路径
            audio_file_path = input("\n请输入音频文件路径 (输入 'quit' 退出): ").strip()
            
            # 退出条件
            if audio_file_path.lower() in ['quit', 'exit', 'q']:
                print("程序退出")
                break
            
            # 处理空输入
            if not audio_file_path:
                print("请输入有效的文件路径")
                continue
            
            # 去除可能的引号
            audio_file_path = audio_file_path.strip('"\'')
            
            # 处理音频文件
            result_text = process_audio_file(audio_file_path)
            
            if result_text:
                print("\n" + "="*50)
                print("识别结果:")
                print(f"文件: {os.path.basename(audio_file_path)}")
                print(f"文本: {result_text}")
                print("="*50)
            else:
                print("转换失败，请检查文件格式和内容")
                
    except KeyboardInterrupt:
        print("\n程序被用户中断")
    except Exception as e:
        print(f"程序运行出错: {str(e)}")