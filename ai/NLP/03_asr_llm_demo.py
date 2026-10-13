import os
import time
import subprocess
import tempfile
import numpy as np

from functions import func_map

from spacemit_llm import LLMModel, FCModel
llm_model = LLMModel()
fc_model = FCModel()

from spacemit_asr import ASRModel
asr_model = ASRModel()

def convert_audio_to_wav(input_file, output_file=None):
    """
    使用ffmpeg将音频文件转换为标准WAV格式
    """
    if output_file is None:
        output_file = tempfile.mktemp(suffix='.wav')
    
    try:
        # 使用ffmpeg转换音频格式为标准WAV
        cmd = [
            'ffmpeg', '-i', input_file, 
            '-ar', '16000',      # 采样率16kHz
            '-ac', '1',          # 单声道
            '-sample_fmt', 's16', # 16位采样
            '-f', 'wav',         # WAV格式
            '-y',                # 覆盖输出文件
            output_file
        ]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode == 0:
            return output_file
        else:
            print(f"ffmpeg转换失败: {result.stderr}")
            return None
    except FileNotFoundError:
        print("错误：未找到ffmpeg")
        print("Windows安装：下载ffmpeg并添加到PATH")
        print("Linux安装：sudo apt-get install ffmpeg")
        return None
    except Exception as e:
        print(f"音频转换出错: {e}")
        return None

def validate_audio_file(file_path):
    """
    验证音频文件是否存在且有效
    """
    if not os.path.exists(file_path):
        return False, f"文件不存在: {file_path}"
    
    file_size = os.path.getsize(file_path)
    if file_size == 0:
        return False, "文件为空"
    
    if file_size < 100:  # 小于100字节可能不是有效音频
        return False, f"文件太小 ({file_size} bytes)，可能不是有效音频文件"
    
    return True, "文件有效"

def process_audio_file(audio_file_path):
    """
    处理音频文件并转换为文本
    """
    # 验证文件
    is_valid, message = validate_audio_file(audio_file_path)
    if not is_valid:
        print(f"文件验证失败: {message}")
        return None
    
    print(f"正在处理音频文件: {os.path.basename(audio_file_path)}")
    print(f"文件大小: {os.path.getsize(audio_file_path)} bytes")
    
    # 检查文件格式
    file_ext = os.path.splitext(audio_file_path)[1].lower()
    converted_file = None
    
    try:
        # 如果不是WAV格式或者WAV格式有问题，尝试转换
        if file_ext != '.wav':
            print(f"检测到 {file_ext} 格式，转换为标准WAV...")
            converted_file = convert_audio_to_wav(audio_file_path)
            if converted_file:
                audio_file_path = converted_file
                print(f"转换成功: {converted_file}")
            else:
                print("格式转换失败")
                return None
        else:
            # 即使是WAV文件，如果出现RIFF错误，也尝试重新转换
            print("WAV文件格式验证...")
            try:
                # 先尝试直接处理
                text = asr_model.generate(audio_file_path)
                return text.strip() if text and text.strip() else None
            except Exception as e:
                if "RIFF" in str(e):
                    print("WAV文件格式有问题，尝试重新转换...")
                    converted_file = convert_audio_to_wav(audio_file_path)
                    if converted_file:
                        audio_file_path = converted_file
                        print(f"重新转换成功: {converted_file}")
                    else:
                        raise e
                else:
                    raise e
        
        # 进行语音识别
        print("开始语音识别...")
        text = asr_model.generate(audio_file_path)
        return text.strip() if text and text.strip() else None
        
    except Exception as e:
        print(f"ASR处理出错: {str(e)}")
        print(f"错误类型: {type(e).__name__}")
        
        # 提供更详细的错误信息和建议
        if "RIFF" in str(e):
            print("建议：")
            print("1. 检查音频文件是否损坏")
            print("2. 尝试使用其他音频文件")
            print("3. 使用 python 02_capture_audio.py 录制新的音频")
            print("4. 确保安装了ffmpeg用于格式转换")
        
        return None
    finally:
        # 清理临时文件
        if converted_file and os.path.exists(converted_file):
            try:
                os.remove(converted_file)
            except:
                pass

def process_user_input(text):
    """
    处理用户输入的文本，调用函数或LLM
    """
    if not text:
        print("未识别到有效文本")
        return
    
    print(f'用户输入: {text}')
    
    # 尝试调用函数
    t1 = time.time()
    function_called = fc_model.func_response(text, func_map)
    print(f'函数调用耗时: {time.time() - t1:.3f}s')
    
    if function_called:
        print("已执行相应的智能家居控制功能")
        return
    
    # 如果没有匹配的函数，使用LLM生成回复
    print("LLM回复: ", end='', flush=True)
    llm_output = llm_model.generate(text)
    for output_text in llm_output:
        print(output_text, end='', flush=True)
    print()  # 换行

if __name__ == '__main__':
    print("=== 智能家居语音助手 (音频文件模式) ===")
    print("支持的功能：灯光、空调、窗帘、电视等智能家居控制")
    print("支持格式：WAV, MP3, FLAC, M4A, OGG等音频文件")
    print("注意：建议使用16kHz采样率的WAV文件以获得最佳效果")
    
    try:
        while True:
            print("\n" + "="*50)
            
            # 输入音频文件路径
            audio_file_path = input("请输入音频文件路径 (输入 'quit' 退出, 'text' 切换到文本模式, 'record' 录制音频): ").strip()
            
            # 退出条件
            if audio_file_path.lower() in ['quit', 'exit', 'q']:
                print("程序退出")
                break
            
            # 录制音频
            if audio_file_path.lower() == 'record':
                print("启动录音程序...")
                print("请运行: python 02_capture_audio.py")
                continue
            
            # 切换到文本输入模式
            if audio_file_path.lower() == 'text':
                print("\n--- 切换到文本输入模式 ---")
                while True:
                    text_input = input("请输入文本指令 (输入 'back' 返回音频模式, 'quit' 退出): ").strip()
                    
                    if text_input.lower() in ['quit', 'exit', 'q']:
                        print("程序退出")
                        exit()
                    
                    if text_input.lower() == 'back':
                        print("返回音频文件模式")
                        break
                    
                    if text_input:
                        process_user_input(text_input)
                continue
            
            # 处理空输入
            if not audio_file_path:
                print("请输入有效的文件路径")
                continue
            
            # 去除可能的引号
            audio_file_path = audio_file_path.strip('"\'')
            
            # 处理音频文件
            text = process_audio_file(audio_file_path)
            
            if text:
                process_user_input(text)
            else:
                print("音频识别失败")
                print("建议：")
                print("1. 输入 'record' 录制新的音频文件")
                print("2. 输入 'text' 切换到文本输入模式进行测试")
                print("3. 检查音频文件路径和格式")
                
    except KeyboardInterrupt:
        print("\n程序被用户中断")
    except Exception as e:
        print(f"程序运行出错: {str(e)}")