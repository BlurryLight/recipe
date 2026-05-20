# 符号加载应该默认使用本地 PDB。除非明确需要系统 DLL 符号，否则不要将
# 微软符号服务器放入 `_NT_SYMBOL_PATH`；否则 `wpaexporter -symbols` 可能
# 会花费很长时间下载 PDB 文件。

# Windows xperf 火焰图示例

此目录使用 xperf 记录 ETW CPU 采样跟踪，用 wpaexporter 导出 CPU 采样堆栈，
将其转换为折叠堆栈，并使用 Brendan Gregg 的 FlameGraph 脚本生成 SVG 火焰图。

默认 pbrt 示例：

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\Invoke-PbrtXperfFlameGraph.ps1 -NoOpen
```

从提升的 PowerShell 会话中运行该命令。xperf 需要管理员权限。

默认目标：

```text
D:\coderepo\cpp-base\pbrt-v4-fork\build\cmake-RelWithDebInfo-visualstudio\pbrt.exe
D:\coderepo\cpp-base\pbrt-v4-fork\scenes\killeroos\killeroo-simple-debug.pbrt
```

输出文件默认写入 `out` 目录：

- `pbrt-killeroo.etl`
- `CPU_Usage_(Sampled)_Randomascii_Export.csv`
- `collapsed_stacks_*.txt`
- `pbrt.exe_*.svg`

需求：

- Windows Performance Toolkit: `xperf.exe` 和 `wpaexporter.exe`
- Python 3.11+
- PATH 中的 Perl，或 `C:\Program Files\Git\usr\bin\perl.exe` 中的 Git for Windows Perl
- `FlameGraph\flamegraph.pl`
- `ExportCPUUsageSampled.wpaProfile`

现有 ETL 文件的手动转换：

```powershell
python .\xperf_to_collapsedstacks.py .\out\pbrt-killeroo.etl -p pbrt.exe -n 8 -o .\out -d
```

符号加载默认关闭，因为 `wpaexporter -symbols` 可能需要花费很长时间解析 PDB。
要仅加载本地 pbrt 符号，请将 `-Symbols` 传递给 PowerShell 脚本，或将 `--symbols --symbol-path <local-pdb-dir-or-file>` 传递给 Python 转换器。

对于 pbrt 示例，`-Symbols` 默认将 `pbrt.exe` 目录作为唯一的 `_NT_SYMBOL_PATH`，
因此不会请求微软符号：

```powershell
.\Invoke-PbrtXperfFlameGraph.ps1 -NoOpen -Symbols
```

覆盖本地符号目录：

```powershell
.\Invoke-PbrtXperfFlameGraph.ps1 -NoOpen -Symbols -SymbolPath D:\path\to\pdbs
```

您也可以传递确切的 PDB 文件；脚本将使用其父目录作为 `wpaexporter` 的 `_NT_SYMBOL_PATH`：

```powershell
.\Invoke-PbrtXperfFlameGraph.ps1 -NoOpen -Symbols -SymbolPath D:\coderepo\cpp-base\pbrt-v4-fork\build\cmake-RelWithDebInfo-visualstudio\pbrt.pdb
```

对于手动 `wpaexporter` 调用，在调用前设置进程本地符号路径并清除备用符号路径：

```powershell
$env:_NT_SYMBOL_PATH = "D:\coderepo\cpp-base\pbrt-v4-fork\build\cmake-RelWithDebInfo-visualstudio"
Remove-Item Env:_NT_ALT_SYMBOL_PATH -ErrorAction SilentlyContinue
wpaexporter .\out\pbrt-killeroo.etl -profile .\ExportCPUUsageSampled.wpaProfile -outputfolder .\out -symbols
```
![README-2026-04-27-18-01-51](https://img.blurredcode.com/img/README-2026-04-27-18-01-51.png?x-oss-process=style/compress)

参考资料：

- https://randomascii.wordpress.com/2013/03/26/summarizing-xperf-cpu-usage-with-flame-graphs/
- https://github.com/google/UIforETW/blob/master/bin/xperf_to_collapsedstacks.py

# macOS sample 火焰图示例

macOS 上可以用系统自带的 `/usr/bin/sample` 对 pbrt 进程采样，再用 Brendan Gregg
FlameGraph 里的 `stackcollapse-sample.awk` 和 `flamegraph.pl` 生成 SVG。

首次使用需要 FlameGraph 脚本：

```bash
git clone --depth 1 https://github.com/brendangregg/FlameGraph.git ./FlameGraph
```

默认目标：

```text
/Users/zhonghaoyu/coderepo/pbrt-v4-fork/build-release/pbrt
/Users/zhonghaoyu/coderepo/pbrt-v4-fork/scenes/killeroos/killeroo-simple-debug.pbrt
```

运行：

```bash
./Invoke-PbrtSampleFlameGraph.sh --duration 5 --interval-ms 1 --spp 64
```

输出文件默认写入 `out-macos`：

- `pbrt-killeroo-sample.sample.txt`
- `pbrt-killeroo-sample.collapsed.txt`
- `pbrt-killeroo-sample.svg`
- `pbrt-killeroo-sample.pbrt.log`
- `pbrt-killeroo-sample.png`

常用覆盖参数：

```bash
./Invoke-PbrtSampleFlameGraph.sh \
  --pbrt /path/to/pbrt \
  --scene /path/to/scene.pbrt \
  --nthreads 8 \
  --spp 128 \
  --duration 10 \
  --interval-ms 1
```

要传额外 pbrt 参数，把它们放在 `--` 后面：

```bash
./Invoke-PbrtSampleFlameGraph.sh --duration 10 -- --wavefront
```

如果 `perl` 因 locale 报错，脚本内部已经对 `flamegraph.pl` 使用 `LC_ALL=C`。

# Android simpleperf 火焰图示例

`Invoke-AndroidSimpleperfFlameGraph.py` 会通过 adb 在设备上调用 simpleperf，
默认采集 `com.Blurredcode.TP_ThirdPerson`：

```powershell
python .\Invoke-AndroidSimpleperfFlameGraph.py --duration 5
```

脚本参考本地 `commands.md` 中验证过的采样命令，默认使用：

```bash
simpleperf record --app com.Blurredcode.TP_ThirdPerson -e task-clock:u -f 99 --duration 5 --call-graph fp -o /data/local/tmp/tp-thirdperson-simpleperf.data
```

输出默认写入 `out-android`：

- `tp-thirdperson-simpleperf.data`
- `tp-thirdperson-simpleperf_symbols.txt`
- `tp-thirdperson-simpleperf_dso.txt`
- `tp-thirdperson-simpleperf_callgraph.txt`
- `tp-thirdperson-simpleperf.samples.txt`
- `tp-thirdperson-simpleperf.collapsed.txt`
- `tp-thirdperson-simpleperf.svg`

常用参数：

```powershell
python .\Invoke-AndroidSimpleperfFlameGraph.py --duration 10 -f 99
python .\Invoke-AndroidSimpleperfFlameGraph.py --include-tid 13818
python .\Invoke-AndroidSimpleperfFlameGraph.py --split-threads --top-threads 5
python .\Invoke-AndroidSimpleperfFlameGraph.py --no-svg
python .\Invoke-AndroidSimpleperfFlameGraph.py --skip-record --keep-remote
```

`--split-threads` 会先采全应用，再按 `report-sample` 里的 `thread_id` 聚合样本，
默认只保留最活跃的前 5 个线程，分别生成线程火焰图：

```powershell
python .\Invoke-AndroidSimpleperfFlameGraph.py --duration 10 --split-threads
```

额外输出：

- `tp-thirdperson-simpleperf_threads.txt`
- `tp-thirdperson-simpleperf_threads\01_tid*_*.collapsed.txt`
- `tp-thirdperson-simpleperf_threads\01_tid*_*.svg`

需求：

- adb 可以连接设备
- 目标应用已启动，且设备上的 simpleperf 支持 `--app`
- PATH 中的 Perl，或 Git for Windows/Strawberry Perl
- `FlameGraph\flamegraph.pl`

如果设备端 simpleperf 不支持 `report-sample`，脚本会保留 `perf.data` 和
simpleperf 文本报告；可通过 `--simpleperf <host-simpleperf>` 指定本机 simpleperf
再尝试生成 SVG。
