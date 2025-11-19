#!/usr/bin/env python3
"""
Android Memory Monitor Tool

A tool to monitor Android app memory usage using adb shell dumpsys.
Collects memory data every second and exports to CSV and SVG line chart.
"""

import argparse
import csv
import subprocess
import sys
import time
import signal
from datetime import datetime
from typing import List, Dict, Optional
import xml.etree.ElementTree as ET

"""Example data

17:20:32 adb shell dumpsys meminfo com.YourCompany.ToolExample
Applications Memory Usage (in Kilobytes):
Uptime: 1277732 Realtime: 1277732

** MEMINFO in pid 5931 [com.YourCompany.ToolExample] **
                   Pss  Private  Private     Swap      Rss     Heap     Heap     Heap
                 Total    Dirty    Clean    Dirty    Total     Size    Alloc     Free
                ------   ------   ------   ------   ------   ------   ------   ------
  Native Heap    21281    21212        0        0    24504    29736    13624    12376
  Dalvik Heap     9138     8940        0        0    16912    27189     2613    24576
 Dalvik Other     1031      972        0        0     1608
        Stack     2236     2236        0        0     2244
       Ashmem       17        0        0        0      592
      Gfx dev    20240    20228       12        0    20240
    Other dev      123        0       16        0      812
     .so mmap   156231     9100   142220        0   198436
    .jar mmap     2257        0      192        0    36636
    .apk mmap      373        0      132        0     1880
    .ttf mmap       34        0        0        0      192
    .dex mmap     2565        4     2544        0     3288
    .oat mmap       99        0        0        0     1928
    .art mmap     6360     5864        0        0    26948
   Other mmap       61       40        4        0      796
   EGL mtrack    30988    30988        0        0    30988
    GL mtrack      960      960        0        0      960
      Unknown   142264   142256        0        0   143040
        TOTAL   396258   242800   145120        0   512004    56925    16237    36952

 App Summary
                       Pss(KB)                        Rss(KB)
                        ------                         ------
           Java Heap:    14804                          43860
         Native Heap:    21212                          24504
                Code:   154192                         242456
               Stack:     2236                           2244
            Graphics:    52188                          52188
       Private Other:   143288
              System:     8338
             Unknown:                                  146752

           TOTAL PSS:   396258            TOTAL RSS:   512004      TOTAL SWAP (KB):        0

 Objects
               Views:       22         ViewRootImpl:        2
         AppContexts:        8           Activities:        2
              Assets:       20        AssetManagers:        0
       Local Binders:       22        Proxy Binders:       44
       Parcel memory:       13         Parcel count:       37
    Death Recipients:        0             WebViews:        0

 SQL
         MEMORY_USED:        0
  PAGECACHE_OVERFLOW:        0          MALLOC_SIZE:        0


"""

class MemoryData:
    """内存数据类"""
    def __init__(self, timestamp: str, total_pss: int, graphics: int,
                 rss: int, heap_size: int, heap_alloc: int):
        self.timestamp = timestamp
        self.total_pss = total_pss
        self.graphics = graphics
        self.rss = rss
        self.heap_size = heap_size
        self.heap_alloc = heap_alloc


class MemoryMonitor:
    """内存监控器"""

    def __init__(self, package_name: str, interval: float = 1.0):
        self.package_name = package_name
        self.interval = interval
        self.memory_data: List[MemoryData] = []
        self.running = False
    
    def stop_monitoring(self):
        """停止监控"""
        self.running = False

    def get_memory_info(self) -> Optional[MemoryData]:
        """获取应用的内存信息"""
        try:
            # 使用 dumpsys meminfo 获取内存信息
            cmd = f"adb shell dumpsys meminfo {self.package_name}"
            result = subprocess.run(cmd, shell=True, capture_output=True, text=True)

            if result.returncode != 0:
                print(f"错误: 无法获取包 {self.package_name} 的内存信息")
                print(f"错误信息: {result.stderr}")
                return None

            output = result.stdout
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

            if "no process found" in output.lower():
                print(f"错误: 未找到包 {self.package_name} 的进程")
                return None

            # 解析内存信息
            lines = output.split('\n')
            total_pss = graphics = rss = heap_size = heap_alloc = 0

            for line in lines:
                if 'TOTAL PSS' in line and 'TOTAL RSS' in line:
                    # 解析 TOTAL PSS 和 TOTAL RSS (同一行)
                    # 格式: TOTAL PSS:   396258            TOTAL RSS:   512004
                    parts = line.split()
                    for i, part in enumerate(parts):
                        if part == 'PSS:' and i + 1 < len(parts):
                            try:
                                total_pss = int(parts[i + 1])
                            except (ValueError, IndexError):
                                pass
                        elif part == 'RSS:' and i + 1 < len(parts):
                            try:
                                rss = int(parts[i + 1])
                            except (ValueError, IndexError):
                                pass
                elif 'Graphics' in line and 'Private' not in line:
                    # 解析 Graphics 信息 (在App Summary部分)
                    # 格式: Graphics:    52188
                    parts = line.split()
                    if len(parts) >= 2 and parts[0] == 'Graphics:':
                        try:
                            graphics = int(parts[1])
                        except ValueError:
                            pass
                elif 'Native Heap' in line:
                    # 解析 Native Heap 信息
                    parts = line.split()
                    if len(parts) >= 6:
                        try:
                            heap_size = int(parts[1]) if parts[1].isdigit() else 0
                            heap_alloc = int(parts[2]) if parts[2].isdigit() else 0
                        except (ValueError, IndexError):
                            pass

            return MemoryData(timestamp, total_pss, graphics, rss, heap_size, heap_alloc)

        except Exception as e:
            print(f"获取内存信息时发生错误: {e}")
            return None

    def start_monitoring(self):
        """开始监控"""
        print(f"开始监控包 {self.package_name} 的内存使用情况...")
        print(f"采集间隔: {self.interval} 秒")
        print("按 Ctrl+C 停止监控并生成报告")
        print("-" * 60)

        self.running = True

        # 设置信号处理
        def signal_handler(signum, frame):
            print("\n正在停止监控...")
            self.running = False

        signal.signal(signal.SIGINT, signal_handler)
        signal.signal(signal.SIGTERM, signal_handler)

        try:
            while self.running:
                memory_info = self.get_memory_info()
                if memory_info:
                    self.memory_data.append(memory_info)
                    # 转换为MB显示
                    pss_mb = memory_info.total_pss / 1024
                    graphics_mb = memory_info.graphics / 1024
                    rss_mb = memory_info.rss / 1024
                    heap_mb = memory_info.heap_alloc / 1024

                    print(f"{memory_info.timestamp} | "
                          f"PSS: {pss_mb:6.1f}MB | "
                          f"Graphics: {graphics_mb:6.1f}MB | "
                          f"RSS: {rss_mb:6.1f}MB | "
                          f"Heap: {heap_mb:6.1f}MB")

                time.sleep(self.interval)

        except KeyboardInterrupt:
            print("\n监控已停止")

        print(f"\n总共采集了 {len(self.memory_data)} 个数据点")

        if self.memory_data:
            self.export_to_csv()
            self.generate_svg_chart()
            print("报告已生成完成!")
        else:
            print("没有采集到数据，不生成报告")

    def export_to_csv(self):
        """导出数据到CSV文件"""
        timestamp_str = datetime.now().strftime("%Y%m%d_%H%M%S")
        csv_filename = f"memory_report_{self.package_name}_{timestamp_str}.csv"

        try:
            with open(csv_filename, 'w', newline='', encoding='utf-8') as csvfile:
                writer = csv.writer(csvfile)

                # 写入表头
                writer.writerow([
                    'Timestamp', 'Total_PSS_KB', 'Graphics_KB',
                    'RSS_KB', 'Heap_Size_KB', 'Heap_Alloc_KB',
                    'Total_PSS_MB', 'Graphics_MB', 'RSS_MB', 'Heap_Alloc_MB'
                ])

                # 写入数据
                for data in self.memory_data:
                    writer.writerow([
                        data.timestamp,
                        data.total_pss,
                        data.graphics,
                        data.rss,
                        data.heap_size,
                        data.heap_alloc,
                        round(data.total_pss / 1024, 2),
                        round(data.graphics / 1024, 2),
                        round(data.rss / 1024, 2),
                        round(data.heap_alloc / 1024, 2)
                    ])

            print(f"CSV报告已保存到: {csv_filename}")

        except Exception as e:
            print(f"导出CSV时发生错误: {e}")

    def generate_svg_chart(self):
        """生成SVG折线图"""
        if not self.memory_data:
            print("没有数据可用于生成图表")
            return

        timestamp_str = datetime.now().strftime("%Y%m%d_%H%M%S")
        svg_filename = f"memory_chart_{self.package_name}_{timestamp_str}.svg"

        try:
            # 图表尺寸
            width, height = 800, 500
            margin = 60
            chart_width = width - 2 * margin
            chart_height = height - 2 * margin

            # 提取数据
            timestamps = [i for i in range(len(self.memory_data))]
            pss_data = [d.total_pss / 1024 for d in self.memory_data]  # 转换为MB
            graphics_data = [d.graphics / 1024 for d in self.memory_data]
            rss_data = [d.rss / 1024 for d in self.memory_data]
            heap_data = [d.heap_alloc / 1024 for d in self.memory_data]

            # 找到数据范围
            all_values = pss_data + graphics_data + rss_data + heap_data
            max_value = max(all_values) if all_values else 1
            min_value = min(all_values) if all_values else 0
            value_range = max_value - min_value

            # 创建SVG
            svg = ET.Element('svg', {
                'width': str(width),
                'height': str(height),
                'xmlns': 'http://www.w3.org/2000/svg'
            })

            # 添加背景
            ET.SubElement(svg, 'rect', {
                'x': '0', 'y': '0', 'width': str(width), 'height': str(height),
                'fill': 'white', 'stroke': 'black'
            })

            # 添加标题
            title = ET.SubElement(svg, 'text', {
                'x': str(width // 2), 'y': '30',
                'text-anchor': 'middle', 'font-size': '18', 'font-weight': 'bold'
            })
            title.text = f'Memory Usage for {self.package_name}'

            # 绘制坐标轴
            # X轴
            ET.SubElement(svg, 'line', {
                'x1': str(margin), 'y1': str(height - margin),
                'x2': str(width - margin), 'y2': str(height - margin),
                'stroke': 'black'
            })

            # Y轴
            ET.SubElement(svg, 'line', {
                'x1': str(margin), 'y1': str(margin),
                'x2': str(margin), 'y2': str(height - margin),
                'stroke': 'black'
            })

            # 添加Y轴刻度和标签
            y_steps = 10
            for i in range(y_steps + 1):
                y_pos = margin + (chart_height * i / y_steps)
                value = max_value - (value_range * i / y_steps)

                # 刻度线
                ET.SubElement(svg, 'line', {
                    'x1': str(margin - 5), 'y1': str(y_pos),
                    'x2': str(margin), 'y2': str(y_pos),
                    'stroke': 'black'
                })

                # 标签
                label = ET.SubElement(svg, 'text', {
                    'x': str(margin - 10), 'y': str(y_pos + 5),
                    'text-anchor': 'end', 'font-size': '12'
                })
                label.text = f'{value:.1f}MB'

            # 添加X轴标签 - 每隔20秒显示一个
            if len(self.memory_data) > 0:
                first_time = datetime.strptime(self.memory_data[0].timestamp, "%Y-%m-%d %H:%M:%S")
                
                # 找出所有需要显示的时间点（每隔20秒）
                x_labels = []
                for i, data in enumerate(self.memory_data):
                    current_time = datetime.strptime(data.timestamp, "%Y-%m-%d %H:%M:%S")
                    seconds_elapsed = int((current_time - first_time).total_seconds())
                    
                    # 每隔20秒显示一个标签
                    if seconds_elapsed % 20 == 0 and (not x_labels or seconds_elapsed != x_labels[-1][0]):
                        x_labels.append((seconds_elapsed, i, data.timestamp))
                
                # 绘制X轴标签
                for seconds, index, time_str in x_labels:
                    if index < len(self.memory_data):
                        x_pos = margin + (chart_width * index / max(1, len(self.memory_data) - 1))
                        label = ET.SubElement(svg, 'text', {
                            'x': str(x_pos), 'y': str(height - margin + 20),
                            'text-anchor': 'middle', 'font-size': '12'
                        })
                        # 显示格式: HH:MM:SS
                        label.text = time_str.split()[1] if ' ' in time_str else time_str

            # 定义颜色
            colors = {
                'PSS': '#FF6B6B',
                'Graphics': '#4ECDC4',
                'RSS': '#45B7D1',
                'Heap': '#FFA07A'
            }

            # 绘制数据线
            datasets = [
                ('PSS', pss_data, colors['PSS']),
                ('Graphics', graphics_data, colors['Graphics']),
                ('RSS', rss_data, colors['RSS']),
                ('Heap', heap_data, colors['Heap'])
            ]

            for name, data, color in datasets:
                if len(data) < 2:
                    continue

                # 创建折线路径
                path_data = []
                for i, value in enumerate(data):
                    x = margin + (chart_width * i / (len(data) - 1))
                    y = margin + chart_height - ((value - min_value) / value_range * chart_height)
                    path_data.append(f"{x},{y}")

                # 绘制折线
                path = ET.SubElement(svg, 'polyline', {
                    'points': ' '.join(path_data),
                    'fill': 'none', 'stroke': color, 'stroke-width': '2'
                })

                # 绘制数据点
                for i, value in enumerate(data):
                    x = margin + (chart_width * i / (len(data) - 1))
                    y = margin + chart_height - ((value - min_value) / value_range * chart_height)
                    ET.SubElement(svg, 'circle', {
                        'cx': str(x), 'cy': str(y), 'r': '3', 'fill': color
                    })

            # 添加图例
            legend_y = height - 30
            legend_items = [
                ('PSS', colors['PSS']),
                ('Graphics', colors['Graphics']),
                ('RSS', colors['RSS']),
                ('Heap', colors['Heap'])
            ]

            legend_x = width - 280
            for i, (name, color) in enumerate(legend_items):
                x = legend_x + (i * 70)

                # 色块
                ET.SubElement(svg, 'rect', {
                    'x': str(x), 'y': str(legend_y), 'width': '15', 'height': '10',
                    'fill': color
                })

                # 标签
                label = ET.SubElement(svg, 'text', {
                    'x': str(x + 20), 'y': str(legend_y + 8),
                    'font-size': '12'
                })
                label.text = name

            # 保存SVG文件
            tree = ET.ElementTree(svg)
            ET.indent(tree, space="  ", level=0)
            tree.write(svg_filename, encoding='utf-8', xml_declaration=True)

            print(f"SVG图表已保存到: {svg_filename}")

        except Exception as e:
            print(f"生成SVG图表时发生错误: {e}")


def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description='Android Memory Monitor - 监控Android应用内存使用情况',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
使用示例:
  python memory_monitor.py com.example.app
  python memory_monitor.py com.android.chrome --interval 0.5
        """
    )

    parser.add_argument('package_name', help='要监控的Android应用包名')
    parser.add_argument(
        '--interval', '-i',
        type=float, default=1.0,
        help='采集间隔时间(秒)，默认1秒'
    )

    args = parser.parse_args()

    # 检查adb是否可用
    try:
        result = subprocess.run(['adb', 'version'],
                              capture_output=True, text=True)
        if result.returncode != 0:
            print("错误: adb命令不可用，请确保Android SDK已正确安装")
            sys.exit(1)
    except FileNotFoundError:
        print("错误: 找不到adb命令，请确保Android SDK已正确安装并添加到PATH")
        sys.exit(1)

    # 创建并启动内存监控器
    monitor = MemoryMonitor(args.package_name, args.interval)
    monitor.start_monitoring()


if __name__ == '__main__':
    main()