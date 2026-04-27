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
