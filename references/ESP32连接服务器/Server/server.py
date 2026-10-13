"""
ESP32-S3 本地大模型服务器
使用 Flask + Ollama 提供 AI 对话服务
模型: qwen2.5:3b
"""

from flask import Flask, request, jsonify, Response
from flask_cors import CORS
import requests
import json
import socket
from datetime import datetime

app = Flask(__name__)
CORS(app)  # 允许跨域请求

# ==================== Flask配置 ====================
app.config['JSON_AS_ASCII'] = False  # 禁用ASCII转义，使用UTF-8编码
app.config['JSONIFY_MIMETYPE'] = 'application/json; charset=utf-8'

# ==================== 配置 ====================
OLLAMA_URL = "http://localhost:11434/api/generate"
MODEL_NAME = "qwen2.5:3b"
PORT = 5000

# ==================== 对话历史（可选）====================
conversation_history = []

def get_local_ip():
    """获取本机局域网IP地址"""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "127.0.0.1"

def call_ollama(prompt, stream=False):
    """
    调用Ollama API生成回复
    """
    payload = {
        "model": MODEL_NAME,
        "prompt": prompt,
        "stream": stream,
        "options": {
            "temperature": 0.7,
            "top_p": 0.9,
            "max_tokens": 500
        }
    }
    
    try:
        response = requests.post(
            OLLAMA_URL,
            json=payload,
            timeout=60
        )
        
        if response.status_code == 200:
            if stream:
                # 处理流式响应
                full_response = ""
                for line in response.iter_lines():
                    if line:
                        data = json.loads(line)
                        if "response" in data:
                            full_response += data["response"]
                        if data.get("done", False):
                            break
                return full_response
            else:
                # 非流式响应
                data = response.json()
                return data.get("response", "")
        else:
            return f"错误: Ollama返回状态码 {response.status_code}"
            
    except requests.exceptions.Timeout:
        return "错误: 请求超时，请检查模型是否正在运行"
    except requests.exceptions.ConnectionError:
        return "错误: 无法连接到Ollama服务，请确保Ollama已启动"
    except Exception as e:
        return f"错误: {str(e)}"

@app.route('/', methods=['GET'])
def home():
    """首页，显示服务状态"""
    return jsonify({
        "status": "running",
        "service": "ESP32 AI Chat Server",
        "model": MODEL_NAME,
        "endpoints": {
            "/chat": "POST - 发送消息获取AI回复",
            "/health": "GET - 检查服务健康状态",
            "/history": "GET - 查看对话历史"
        }
    })

@app.route('/health', methods=['GET'])
def health_check():
    """健康检查端点"""
    try:
        # 测试Ollama连接
        test_response = requests.get("http://localhost:11434/api/tags", timeout=5)
        ollama_status = "connected" if test_response.status_code == 200 else "disconnected"
    except:
        ollama_status = "disconnected"
    
    return jsonify({
        "status": "healthy",
        "ollama": ollama_status,
        "model": MODEL_NAME,
        "timestamp": datetime.now().isoformat()
    })

@app.route('/chat', methods=['POST'])
def chat():
    """
    接收ESP32发送的消息，调用Ollama生成回复
    """
    try:
        # 获取请求数据
        data = request.get_json()
        
        if not data or 'message' not in data:
            return jsonify({
                "error": "缺少message字段"
            }), 400
        
        user_message = data['message']
        print(f"\n[{datetime.now().strftime('%H:%M:%S')}] 收到消息: {user_message}")
        
        # 调用Ollama生成回复
        ai_response = call_ollama(user_message, stream=False)
        print(f"[{datetime.now().strftime('%H:%M:%S')}] AI回复: {ai_response[:100]}...")
        
        # 保存到历史记录
        conversation_history.append({
            "timestamp": datetime.now().isoformat(),
            "user": user_message,
            "ai": ai_response
        })
        
        # 限制历史记录长度
        if len(conversation_history) > 50:
            conversation_history.pop(0)
        
        # 手动构建JSON响应，强制使用UTF-8编码
        response_data = {
            "success": True,
            "response": ai_response,
            "timestamp": datetime.now().isoformat()
        }
        
        # 使用 ensure_ascii=False 禁用Unicode转义
        json_str = json.dumps(response_data, ensure_ascii=False)
        
        return Response(
            json_str,
            mimetype='application/json; charset=utf-8',
            status=200
        )
        
    except Exception as e:
        print(f"错误: {str(e)}")
        error_data = {
            "success": False,
            "error": str(e)
        }
        json_str = json.dumps(error_data, ensure_ascii=False)
        return Response(
            json_str,
            mimetype='application/json; charset=utf-8',
            status=500
        )

@app.route('/history', methods=['GET'])
def get_history():
    """获取对话历史"""
    return jsonify({
        "history": conversation_history,
        "count": len(conversation_history)
    })

@app.route('/clear', methods=['POST'])
def clear_history():
    """清空对话历史"""
    conversation_history.clear()
    return jsonify({
        "success": True,
        "message": "对话历史已清空"
    })

if __name__ == '__main__':
    print("=" * 60)
    print("ESP32-S3 本地大模型服务器启动中...")
    print("=" * 60)
    print(f"模型: {MODEL_NAME}")
    print(f"端口: {PORT}")
    
    local_ip = get_local_ip()
    print(f"\n局域网访问地址: http://{local_ip}:{PORT}")
    print(f"本地访问地址: http://localhost:{PORT}")
    print(f"\n请在ESP32代码中设置服务器地址为:")
    print(f'  String serverURL = "http://{local_ip}:{PORT}/chat";')
    print("\n" + "=" * 60)
    print("服务器运行中，按 Ctrl+C 停止...")
    print("=" * 60 + "\n")
    
    # 启动Flask服务器
    app.run(
        host='0.0.0.0',  # 允许局域网访问
        port=PORT,
        debug=False,
        threaded=True
    )
