import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from datetime import datetime
import csv
import re
import threading
import matplotlib
# 配置matplotlib支持中文显示
matplotlib.rcParams['font.family'] = ['SimHei', 'Microsoft YaHei', 'SimSun', 'sans-serif']
matplotlib.rcParams['axes.unicode_minus'] = False  # 解决负号显示问题

# 全局数据存储
data = {
    'time': [],
    'voc': [],
    'hcho': [],
    'pm25': []
}

# 超标阈值
thresholds = {
    'voc': 600,
    'hcho': 100,
    'pm25': 75
}

# 串口配置（根据实际情况修改端口）
ser = serial.Serial('COM3', 115200, timeout=1)

def parse_line(line):
    """解析传感器数据行"""
    match = re.search(r'VOC: (\d+).*?甲醛: (\d+).*?PM2\.5: (\d+)', line)
    if match:
        return (
            int(match.group(1)),
            int(match.group(2)),
            int(match.group(3))
        )
    return None

def save_data():
    """定时保存数据到CSV"""
    if data['time']:
        filename = datetime.now().strftime('sensor_data_%Y%m%d_%H%M%S.csv')
        with open(filename, 'w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow(['Timestamp', 'VOC(ug/m³)', 'HCHO(ug/m³)', 'PM2.5(ug/m³)'])
            for t, voc, hcho, pm25 in zip(data['time'], data['voc'], data['hcho'], data['pm25']):
                writer.writerow([t.isoformat(), voc, hcho, pm25])
        print(f'数据已保存至 {filename}')
    # 重置定时器
    threading.Timer(300, save_data).start()

def animate(i, ax1, ax2, ax3):
    """动画更新函数"""
    # 读取所有可用数据
    while ser.in_waiting:
        try:
            line = ser.readline().decode('utf-8').strip()
            if parsed := parse_line(line):
                voc, hcho, pm25 = parsed
                data['time'].append(datetime.now())
                data['voc'].append(voc)
                data['hcho'].append(hcho)
                data['pm25'].append(pm25)
        except UnicodeDecodeError:
            continue

    # 清空旧绘图
    ax1.clear()
    ax2.clear()
    ax3.clear()

    if not data['time']:
        return []

    # 计算相对时间（秒）
    base_time = data['time'][0]
    rel_times = [(t - base_time).total_seconds() for t in data['time']]

    # 绘制VOC
    ax1.set_title('VOC 实时监测')
    ax1.set_ylabel('浓度 (ug/m³)')
    ax1.plot(rel_times, data['voc'], 'g-', alpha=0.3)
    for t, val in zip(rel_times, data['voc']):
        ax1.scatter(t, val, color='red' if val > thresholds['voc'] else 'green', s=10)
    ax1.axhline(thresholds['voc'], color='r', linestyle='--', linewidth=1)
    ax1.set_ylim(0, thresholds['voc'] * 7.0)

    # 绘制甲醛
    ax2.set_title('甲醛 实时监测')
    ax2.set_ylabel('浓度 (ug/m³)')
    ax2.plot(rel_times, data['hcho'], 'g-', alpha=0.3)
    for t, val in zip(rel_times, data['hcho']):
        ax2.scatter(t, val, color='red' if val > thresholds['hcho'] else 'green', s=10)
    ax2.axhline(thresholds['hcho'], color='r', linestyle='--', linewidth=1)
    ax2.set_ylim(0, thresholds['hcho'] * 7.0)

    # 绘制PM2.5
    ax3.set_title('PM2.5 实时监测')
    ax3.set_xlabel('时间 (秒)')
    ax3.set_ylabel('浓度 (ug/m³)')
    ax3.plot(rel_times, data['pm25'], 'g-', alpha=0.3)
    for t, val in zip(rel_times, data['pm25']):
        ax3.scatter(t, val, color='red' if val > thresholds['pm25'] else 'green', s=10)
    ax3.axhline(thresholds['pm25'], color='r', linestyle='--', linewidth=1)
    ax3.set_ylim(0, thresholds['pm25'] * 7.0)

    plt.tight_layout()
    return []

# 初始化图表
fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 12))
fig.suptitle('室内空气质量实时监测', fontsize=14)

# 启动定时保存
threading.Timer(300, save_data).start()

# 启动动画
ani = animation.FuncAnimation(
    fig, 
    animate, 
    fargs=(ax1, ax2, ax3),
    interval=1000,
    cache_frame_data=False
)

plt.show()
