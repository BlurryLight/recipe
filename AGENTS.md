# AGENTS.md

给在本仓库中工作的 agent（以及人类协作者）的约定。`CLAUDE.md` 是 Claude Code 专用的等价文件，二者内容保持一致即可。

## Python 环境（python_examples）

任何 Python 依赖都必须隔离在 `python_examples/.venv` 虚拟环境里，**不要**直接装到 user 目录或系统 site-packages。

- 创建/更新虚拟环境用 `python_examples/setup_venv.sh`，它会装好 `requirements.txt` 里的包。
- 允许用 `uv` / `venv` / `pyenv` 等工具来管理虚拟环境。
- 如果当前机器上**缺少这类工具**（比如没有 `uv`），可以把**工具本身**（例如 `uv`）安装到 user 目录（`~/.local/bin`）。但**除工具之外的任何包**（numpy、matplotlib、以及任何第三方库）一律只装进 `python_examples/.venv`，禁止装进 user / 系统 site-packages。
- 本机的 Python 通常是 externally-managed（PEP 668），直接 `pip install` 会被拒绝——这是预期的，正是逼你走虚拟环境。不要用 `--break-system-packages` 绕过。
- `python_examples/.venv/` 已在 `.gitignore` 里，不要提交。
