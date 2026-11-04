import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch

# 设置中文字体
plt.rcParams['font.sans-serif'] = ['SimHei', 'Microsoft YaHei', 'Arial Unicode MS']
plt.rcParams['axes.unicode_minus'] = False

# 创建画布
fig, ax = plt.subplots(figsize=(16, 12))
ax.set_xlim(0, 16)
ax.set_ylim(0, 12)
ax.axis('off')

# 定义颜色
colors = {
    'root': '#FF6B6B',       # 根节点 - 红色
    'layer1': '#4ECDC4',     # 第一层 - 青色
    'layer2': '#45B7D1',     # 第二层 - 蓝色
    'layer3': '#96CEB4',     # 第三层 - 绿色
    'layer4': '#FFEAA7',     # 第四层 - 黄色
    'text': '#2D3436'        # 文字颜色
}

# 定义函数：创建节点
def create_node(x, y, width, height, text, color, fontsize=10, fontweight='normal'):
    box = FancyBboxPatch((x, y), width, height, 
                         boxstyle="round,pad=0.05", 
                         facecolor=color, 
                         edgecolor='black', 
                         linewidth=1.5)
    ax.add_patch(box)
    ax.text(x + width/2, y + height/2, text, 
            ha='center', va='center', 
            fontsize=fontsize, 
            fontweight=fontweight,
            color=colors['text'],
            wrap=True)

# 定义函数：创建连接线
def create_line(x1, y1, x2, y2, color='#636e72'):
    ax.plot([x1, x2], [y1, y2], color=color, linewidth=1.5, zorder=1)

# ========== 根节点 ==========
create_node(6, 10.5, 4, 0.8, 'IoT育苗控制系统', colors['root'], 14, 'bold')

# ========== 第一层：五大模块 ==========
# 用户层
create_node(0.5, 8.5, 2.5, 0.6, '用户层', colors['layer1'], 11, 'bold')
create_line(2.25, 9.3, 7.5, 10.5)

# 应用层
create_node(3.5, 8.5, 2.5, 0.6, '应用层', colors['layer1'], 11, 'bold')
create_line(4.75, 9.1, 8, 10.5)

# 云平台层
create_node(6.5, 8.5, 2.5, 0.6, '云平台层', colors['layer1'], 11, 'bold')
create_line(7.75, 9.1, 8, 10.5)

# 硬件层
create_node(9.5, 8.5, 2.5, 0.6, '硬件层', colors['layer1'], 11, 'bold')
create_line(10.75, 9.1, 8, 10.5)

# 数据库层
create_node(12.5, 8.5, 2.5, 0.6, '数据库层', colors['layer1'], 11, 'bold')
create_line(13.75, 9.3, 8.5, 10.5)

# ========== 第二层：各模块详细功能 ==========

# 用户层子模块
create_node(0.2, 7, 3.1, 0.5, '管理员', colors['layer2'], 10)
create_line(1.75, 8.5, 1.75, 7.5)

create_node(0.2, 6, 3.1, 0.5, '普通用户', colors['layer2'], 10)
create_line(1.75, 7, 1.75, 6.5)

# 应用层子模块
create_node(3.2, 7, 3.1, 0.5, '微信小程序', colors['layer2'], 10)
create_line(4.75, 8.5, 4.75, 7.5)

create_node(3.2, 6, 3.1, 0.5, '树莓派中控', colors['layer2'], 10)
create_line(4.75, 7, 4.75, 6.5)

create_node(3.2, 5, 3.1, 0.5, '语音控制', colors['layer2'], 10)
create_line(4.75, 6, 4.75, 5.5)

# 云平台层子模块
create_node(6.2, 7, 3.1, 0.5, '华为云IoT', colors['layer2'], 10)
create_line(7.75, 8.5, 7.75, 7.5)

create_node(6.2, 6, 3.1, 0.5, 'MQTT通信', colors['layer2'], 10)
create_line(7.75, 7, 7.75, 6.5)

# 硬件层子模块
create_node(9.2, 7, 3.1, 0.5, '传感器模块', colors['layer2'], 10)
create_line(10.75, 8.5, 10.75, 7.5)

create_node(9.2, 6, 3.1, 0.5, '控制器模块', colors['layer2'], 10)
create_line(10.75, 7, 10.75, 6.5)

create_node(9.2, 5, 3.1, 0.5, '执行器模块', colors['layer2'], 10)
create_line(10.75, 6, 10.75, 5.5)

create_node(9.2, 4, 3.1, 0.5, '显示模块', colors['layer2'], 10)
create_line(10.75, 5, 10.75, 4.5)

# 数据库层子模块
create_node(12.2, 7, 3.1, 0.5, '用户表', colors['layer2'], 10)
create_line(13.75, 8.5, 13.75, 7.5)

create_node(12.2, 6, 3.1, 0.5, '环境数据表', colors['layer2'], 10)
create_line(13.75, 7, 13.75, 6.5)

create_node(12.2, 5, 3.1, 0.5, '操作日志表', colors['layer2'], 10)
create_line(13.75, 6, 13.75, 5.5)

# ========== 第三层：具体功能 ==========

# 微信小程序功能
create_node(3, 3.5, 3.5, 0.4, '登录/注册', colors['layer3'], 9)
create_line(4.75, 5, 4.75, 3.9)

create_node(3, 2.9, 3.5, 0.4, '设备监控', colors['layer3'], 9)
create_line(4.75, 3.5, 4.75, 3.3)

create_node(3, 2.3, 3.5, 0.4, '设备控制', colors['layer3'], 9)
create_line(4.75, 2.9, 4.75, 2.7)

create_node(3, 1.7, 3.5, 0.4, '阈值设置', colors['layer3'], 9)
create_line(4.75, 2.3, 4.75, 2.1)

# 传感器具体类型
create_node(9, 3, 3.5, 0.4, '温湿度传感器', colors['layer3'], 9)
create_line(10.75, 5, 10.75, 3.4)

create_node(9, 2.4, 3.5, 0.4, '光敏传感器', colors['layer3'], 9)
create_line(10.75, 3, 10.75, 2.8)

create_node(9, 1.8, 3.5, 0.4, '土壤湿度传感器', colors['layer3'], 9)
create_line(10.75, 2.4, 10.75, 2.2)

# ========== 标题 ==========
ax.text(8, 11.5, 'IoT育苗控制系统结构树状图', 
        ha='center', va='center', 
        fontsize=20, fontweight='bold',
        color=colors['text'])

# ========== 图例 ==========
legend_elements = [
    mpatches.Patch(color=colors['root'], label='系统根节点'),
    mpatches.Patch(color=colors['layer1'], label='系统层级'),
    mpatches.Patch(color=colors['layer2'], label='功能模块'),
    mpatches.Patch(color=colors['layer3'], label='具体功能')
]

ax.legend(handles=legend_elements, loc='upper right', 
          bbox_to_anchor=(0.98, 0.98), 
          fontsize=9,
          frameon=True,
          framealpha=0.8)

# 调整布局
plt.tight_layout()

# 保存图片
plt.savefig('系统结构树状图.png', dpi=300, bbox_inches='tight', 
            facecolor='white', edgecolor='none')

print("系统结构树状图已生成：系统结构树状图.png")
plt.show()
