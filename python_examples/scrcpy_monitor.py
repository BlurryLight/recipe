#!/usr/bin/env python3
"""
Android Screen Recorder with scrcpy

A lightweight Android screen recording tool using scrcpy with minimal resource usage.
Records app activity with timestamp-based file naming.
"""

import argparse
import os
import subprocess
import sys
import time
import signal
import shutil
from datetime import datetime
from pathlib import Path
from typing import Optional, List


class ScrcpyMonitor:
    """scrcpy监控录像器"""

    def __init__(self,
                 max_fps: int = 4,
                 resolution: str = "720",
                 bit_rate: str = "500k",
                 output_dir: str = "recordings",
                 duration: Optional[int] = None,
                 no_display: bool = True):
        """
        初始化scrcpy监控器

        Args:
            max_fps: 最大帧率，默认4fps（低开销）
            resolution: 分辨率，默认720
            bit_rate: 视频比特率，默认500k
            output_dir: 输出目录
            duration: 录制时长（秒），None表示持续录制
            no_display: 是否不显示预览窗口
        """
        self.max_fps = max_fps
        self.resolution = resolution
        self.bit_rate = bit_rate
        self.output_dir = output_dir
        self.duration = duration
        self.no_display = no_display

        self.process: Optional[subprocess.Popen] = None
        self.running = False
        self.start_time = None

        # 创建输出目录
        Path(self.output_dir).mkdir(parents=True, exist_ok=True)

    def generate_filename(self) -> str:
        """生成带时间戳的文件名"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        return f"screen_record_{timestamp}.mp4"

    def get_scrcpy_command(self, filename: str) -> List[str]:
        """构建scrcpy命令"""
        cmd = [
            "scrcpy",
            "--record", filename,
            "--max-fps", str(self.max_fps),
            "-m", self.resolution,
            "--video-bit-rate", self.bit_rate,
            "--no-playback",
            "--no-audio"
        ]

        # 如果不显示预览窗口，添加相关参数
        if self.no_display:
            cmd.extend(["--window-title", "Recording", "--window-x", "-9999"])

        return cmd

    def check_scrcpy_available(self) -> bool:
        """检查scrcpy是否可用"""
        try:
            result = subprocess.run(
                ["scrcpy", "--version"],
                capture_output=True,
                text=True,
                timeout=5
            )
            return result.returncode == 0
        except (subprocess.TimeoutExpired, FileNotFoundError):
            return False

    def check_device_connected(self) -> bool:
        """检查Android设备是否连接"""
        try:
            result = subprocess.run(
                ["adb", "devices"],
                capture_output=True,
                text=True,
                timeout=5
            )
            return "device" in result.stdout and result.stdout.count("\tdevice") > 0
        except (subprocess.TimeoutExpired, FileNotFoundError):
            return False

    def start_recording(self, app_package: Optional[str] = None) -> bool:
        """开始录制"""
        if not self.check_scrcpy_available():
            print("错误: scrcpy命令不可用，请确保已安装scrcpy")
            return False

        if not self.check_device_connected():
            print("错误: 未找到连接的Android设备")
            return False

        # 如果指定了应用包名，尝试启动应用
        if app_package:
            self.start_app(app_package)

        filename = os.path.join(self.output_dir, self.generate_filename())
        cmd = self.get_scrcpy_command(filename)

        print(f"开始录制屏幕...")
        print(f"输出文件: {filename}")
        print(f"配置: {self.max_fps}fps, {self.resolution}p, {self.bit_rate}bps")
        print(f"按 Ctrl+C 停止录制")
        print("-" * 60)

        try:
            self.process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1,
                universal_newlines=True
            )

            self.running = True
            self.start_time = time.time()

            # 设置信号处理
            signal.signal(signal.SIGINT, self._signal_handler)
            signal.signal(signal.SIGTERM, self._signal_handler)

            # 监控录制进程
            self._monitor_process()

        except Exception as e:
            print(f"启动录制时发生错误: {e}")
            return False

        return True

    def start_app(self, package_name: str) -> bool:
        """启动指定的Android应用"""
        try:
            cmd = ["adb", "shell", "monkey", "-p", package_name, "-c", "android.intent.category.LAUNCHER", "1"]
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)

            if result.returncode == 0:
                print(f"已启动应用: {package_name}")
                time.sleep(2)  # 等待应用启动
                return True
            else:
                print(f"警告: 无法启动应用 {package_name}")
                return False

        except subprocess.TimeoutExpired:
            print(f"警告: 启动应用 {package_name} 超时")
            return False
        except Exception as e:
            print(f"启动应用时发生错误: {e}")
            return False

    def _monitor_process(self):
        """监控录制进程"""
        last_status_time = time.time()

        while self.running and self.process:
            # 检查进程是否还在运行
            if self.process.poll() is not None:
                stdout, stderr = self.process.communicate()
                if self.process.returncode != 0:
                    print(f"录制进程异常退出: {stderr}")
                else:
                    print("录制正常完成")
                break

            # 检查录制时长限制
            if self.duration and self.start_time:
                elapsed = time.time() - self.start_time
                if elapsed >= self.duration:
                    print(f"达到指定录制时长 {self.duration} 秒，停止录制")
                    self.stop_recording()
                    break

            # 每30秒显示一次状态
            current_time = time.time()
            if current_time - last_status_time >= 30:
                if self.start_time:
                    elapsed = current_time - self.start_time
                    print(f"录制中... 已录制 {elapsed:.0f} 秒")
                last_status_time = current_time

            time.sleep(1)

    def _signal_handler(self, signum, frame):
        """信号处理器"""
        print("\n收到停止信号，正在停止录制...")
        self.stop_recording()

    def stop_recording(self):
        """停止录制"""
        self.running = False

        if self.process:
            try:
                # 发送SIGTERM信号
                self.process.terminate()

                # 等待进程结束，最多等待5秒
                try:
                    self.process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    # 如果5秒后还没结束，强制杀死
                    print("强制结束录制进程...")
                    self.process.kill()
                    self.process.wait()

                print("录制已停止")

            except Exception as e:
                print(f"停止录制时发生错误: {e}")

    def cleanup_old_files(self, keep_days: int = 7):
        """清理旧的录制文件"""
        try:
            cutoff_time = time.time() - (keep_days * 24 * 3600)
            cleaned_count = 0

            for file_path in Path(self.output_dir).glob("screen_record_*.mp4"):
                if file_path.stat().st_mtime < cutoff_time:
                    file_path.unlink()
                    cleaned_count += 1

            if cleaned_count > 0:
                print(f"已清理 {cleaned_count} 个超过 {keep_days} 天的旧录制文件")

        except Exception as e:
            print(f"清理旧文件时发生错误: {e}")


def print_system_info():
    """打印系统信息"""
    print("=== 系统信息 ===")

    # 检查scrcpy版本
    try:
        result = subprocess.run(["scrcpy", "--version"],
                              capture_output=True, text=True)
        if result.returncode == 0:
            print(f"scrcpy版本: {result.stdout.strip()}")
        else:
            print("警告: 无法获取scrcpy版本信息")
    except:
        print("错误: scrcpy未安装或不可用")

    # 检查adb设备
    try:
        result = subprocess.run(["adb", "devices"],
                              capture_output=True, text=True)
        devices = [line for line in result.stdout.split('\n')
                  if '\tdevice' in line]
        print(f"连接的设备数量: {len(devices)}")
        for device in devices:
            device_id = device.split('\t')[0]
            print(f"  设备ID: {device_id}")
    except:
        print("警告: 无法检查adb设备状态")

    print("-" * 60)


def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description='Android Screen Recorder with scrcpy',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
使用示例:
  # 基本录制
  python scrcpy_monitor.py

  # 录制指定应用
  python scrcpy_monitor.py --app com.example.app

  # 自定义录制参数
  python scrcpy_monitor.py --fps 2 --resolution 480 --bit-rate 300k

  # 限制录制时长
  python scrcpy_monitor.py --duration 60

  # 显示预览窗口
  python scrcpy_monitor.py --display
        """
    )

    parser.add_argument(
        '--app', '-a',
        help='启动指定包名的Android应用'
    )

    parser.add_argument(
        '--fps', '-f',
        type=int, default=4,
        help='录制帧率 (默认: 4，推荐2-6以降低开销)'
    )

    parser.add_argument(
        '--resolution', '-r',
        default='720',
        help='录制分辨率 (默认: 720)'
    )

    parser.add_argument(
        '--bit-rate', '-b',
        default='500k',
        help='视频比特率 (默认: 500k)'
    )

    parser.add_argument(
        '--output-dir', '-o',
        default='recordings',
        help='输出目录 (默认: recordings)'
    )

    parser.add_argument(
        '--duration', '-d',
        type=int,
        help='录制时长(秒)，不指定则持续录制直到手动停止'
    )

    parser.add_argument(
        '--display',
        action='store_true',
        help='显示预览窗口 (默认不显示以降低资源占用)'
    )

    parser.add_argument(
        '--cleanup',
        type=int,
        metavar='DAYS',
        help='清理超过指定天数的旧录制文件'
    )

    args = parser.parse_args()

    # 打印系统信息
    print_system_info()

    # 创建监控器
    monitor = ScrcpyMonitor(
        max_fps=args.fps,
        resolution=args.resolution,
        bit_rate=args.bit_rate,
        output_dir=args.output_dir,
        duration=args.duration,
        no_display=not args.display
    )

    # 清理旧文件
    if args.cleanup:
        monitor.cleanup_old_files(args.cleanup)
        return

    # 开始录制
    success = monitor.start_recording(args.app)

    if not success:
        print("录制启动失败")
        sys.exit(1)

    print("录制程序已退出")


if __name__ == '__main__':
    main()