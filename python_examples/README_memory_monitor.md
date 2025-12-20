# Android 内存监控工具

基于 `adb shell dumpsys` 的Android应用内存监控工具，可以实时采集应用内存数据并生成CSV报告和SVG折线图。

## 功能特性

- 📊 实时监控Android应用内存使用情况
- ⏱️ 可自定义采集间隔（默认1秒）
- 📈 支持多种内存指标：PSS、USS、RSS、Heap
- 📁 自动生成CSV数据报告
- 🎨 生成可视化SVG折线图
- 🛡️ 优雅的信号处理，支持Ctrl+C退出
- 🔍 详细的错误处理和状态反馈

## 安装要求

- Python 3.6+
- Android SDK
- adb 工具已添加到系统PATH

## 使用方法

### 基本用法

```bash
# 监控指定包名（默认每秒采集一次）
python memory_monitor.py com.example.app

# 自定义采集间隔
python memory_monitor.py com.android.chrome --interval 0.5

# 查看帮助信息
python memory_monitor.py -h
```

### 命令行参数

- `package_name` (必需): 要监控的Android应用包名
- `--interval`, `-i` (可选): 采集间隔时间（秒），默认1.0秒

## 输出文件

工具会生成两种输出文件：

### 1. CSV数据报告
文件名格式：`memory_report_{包名}_{时间戳}.csv`

包含以下列：
- Timestamp: 采集时间戳
- Total_PSS_KB: 总PSS内存（KB）
- Total_USS_KB: 总USS内存（KB）
- RSS_KB: RSS内存（KB）
- Heap_Size_KB: 堆大小（KB）
- Heap_Alloc_KB: 堆分配（KB）
- 以及对应的MB单位数据

### 2. SVG可视化图表
文件名格式：`memory_chart_{包名}_{时间戳}.svg`

显示四种内存指标的变化趋势：
- **PSS (Proportional Set Size)**: 红色线条
- **USS (Unique Set Size)**: 青色线条
- **RSS (Resident Set Size)**: 蓝色线条
- **Heap**: 橙色线条

## 内存指标说明

- **PSS**: 包含共享库按比例分摊的内存使用量
- **USS**: 应用独占的内存使用量（不包含共享库）
- **RSS**: 物理内存使用量
- **Heap**: Java堆内存分配情况

## 使用示例

```bash
# 监控Chrome浏览器，每0.5秒采集一次
python memory_monitor.py com.android.chrome --interval 0.5

# 监控微信应用，使用默认1秒间隔
python memory_monitor.py com.tencent.mm
```

## 注意事项

1. 确保设备已连接并启用USB调试
2. 确保目标应用正在运行
3. 按Ctrl+C可随时停止监控并生成报告
4. 生成的文件会保存在当前目录下
5. 如果目标应用不存在或未运行，工具会显示错误信息

## 故障排除

### adb命令不可用
```bash
# 检查adb是否安装
adb version

# 如果未安装，请安装Android SDK并将adb添加到PATH
```

### 找不到包名
```bash
# 查看当前运行的包名
adb shell dumpsys activity top | grep "ACTIVITY"

# 或使用pm命令查看已安装应用
adb shell pm list packages
```

### 权限问题
确保adb有足够权限访问目标应用的内存信息，某些设备可能需要root权限。

## 技术实现

- 使用 `subprocess` 模块执行adb命令
- 通过 `dumpsys meminfo` 获取详细内存信息
- 使用 `xml.etree.ElementTree` 生成SVG图表
- 支持信号处理实现优雅退出
- 实时显示采集进度和内存使用情况