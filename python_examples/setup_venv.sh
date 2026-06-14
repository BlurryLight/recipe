#!/usr/bin/env bash
# =============================================================================
# 为 python_examples 创建/更新一个独立的 Python 虚拟环境（./.venv）。
#
# 设计原则（详见仓库根目录 AGENTS.md）：
#   - 优先使用 uv（快、可复现）；如果没有，会把 uv 这个 *工具* 安装到 user 目录
#     （~/.local/bin），但绝不把 numpy / matplotlib 等 *包* 装到 user / 系统 site-packages。
#   - 所有依赖包只活在 ./python_examples/.venv 里。
#   - 在没有 uv、且无法联网安装 uv 的机器上，回退到标准库的 `python3 -m venv`。
#
# 用法（clone 到新机器后）：
#   bash python_examples/setup_venv.sh
# 之后激活：
#   source python_examples/.venv/bin/activate
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

VENV_DIR=".venv"
REQ_FILE="requirements.txt"
USER_BIN="${XDG_BIN_HOME:-$HOME/.local/bin}"

# -----------------------------------------------------------------------------
# 1. 选一个可用的 python 解释器（仅用来驱动 uv 或 stdlib venv，不会被污染）
# -----------------------------------------------------------------------------
if command -v python3 >/dev/null 2>&1; then
  PY="python3"
elif command -v python >/dev/null 2>&1; then
  PY="python"
else
  echo "ERROR: 未找到 python 解释器，请先安装 Python 3。" >&2
  exit 1
fi
echo ">> 使用解释器: $($PY -c 'import sys; print(sys.executable, sys.version.split()[0])')"

# -----------------------------------------------------------------------------
# 2. 确保 uv 可用；缺失则把 uv 工具安装到 user 目录（$USER_BIN）
# -----------------------------------------------------------------------------
have_uv() { command -v uv >/dev/null 2>&1; }

if ! have_uv; then
  echo ">> 未检测到 uv，开始安装 uv 工具到 $USER_BIN ..."
  mkdir -p "$USER_BIN"
  install_script="$(mktemp)"
  if command -v curl >/dev/null 2>&1; then
    curl -LsSf https://astral.sh/uv/install.sh -o "$install_script"
  elif command -v wget >/dev/null 2>&1; then
    wget -qO "$install_script" https://astral.sh/uv/install.sh
  else
    echo "ERROR: 需要 curl 或 wget 来下载 uv 安装脚本。" >&2
    exit 1
  fi
  # 安装到 user 目录，不影响系统环境；禁用修改 shell rc 也行，这里保留默认。
  sh "$install_script"
  rm -f "$install_script"
  export PATH="$USER_BIN:$PATH"
fi

# -----------------------------------------------------------------------------
# 3. 创建虚拟环境 + 安装依赖
# -----------------------------------------------------------------------------
if have_uv; then
  echo ">> 使用 uv 创建虚拟环境 ..."
  # --python 指定用上面找到的系统解释器，避免 uv 自作主张去下载别的版本。
  uv venv --python "$PY" "$VENV_DIR"
  uv pip install --python "$VENV_DIR/bin/python" -r "$REQ_FILE"
else
  echo ">> uv 不可用，回退到标准库 venv ..."
  "$PY" -m venv "$VENV_DIR"
  "$VENV_DIR/bin/python" -m pip install --upgrade pip
  "$VENV_DIR/bin/python" -m pip install -r "$REQ_FILE"
fi

echo
echo "=========================================================="
echo "✅ 虚拟环境就绪: $SCRIPT_DIR/$VENV_DIR"
echo "   激活:  source $VENV_DIR/bin/activate"
echo "   退出:  deactivate"
echo "=========================================================="
