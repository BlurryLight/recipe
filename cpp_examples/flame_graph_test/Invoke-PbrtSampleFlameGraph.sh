#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

PBRT_EXE="${PBRT_EXE:-/Users/zhonghaoyu/coderepo/pbrt-v4-fork/build-release/pbrt}"
SCENE="${SCENE:-/Users/zhonghaoyu/coderepo/pbrt-v4-fork/scenes/killeroos/killeroo-simple-debug.pbrt}"
OUT_DIR="${OUT_DIR:-${SCRIPT_DIR}/out-macos}"
TRACE_NAME="${TRACE_NAME:-pbrt-killeroo-sample}"
NTHREADS="${NTHREADS:-8}"
SPP="${SPP:-64}"
SAMPLE_DURATION="${SAMPLE_DURATION:-5}"
SAMPLE_INTERVAL_MS="${SAMPLE_INTERVAL_MS:-1}"
FLAMEGRAPH_DIR="${FLAMEGRAPH_DIR:-${SCRIPT_DIR}/FlameGraph}"
OPEN_SVG="${OPEN_SVG:-0}"

usage() {
    cat <<EOF
Usage:
  $(basename "$0") [options] [-- extra pbrt args...]

Options:
  --pbrt PATH              pbrt executable. Default: ${PBRT_EXE}
  --scene PATH             scene file. Default: ${SCENE}
  --out-dir DIR            output directory. Default: ${OUT_DIR}
  --trace-name NAME        output file prefix. Default: ${TRACE_NAME}
  --nthreads N             pbrt --nthreads value. Default: ${NTHREADS}
  --spp N                  pbrt --spp value. Default: ${SPP}
  --duration SECONDS       sample duration. Default: ${SAMPLE_DURATION}
  --interval-ms MS         sample interval. Default: ${SAMPLE_INTERVAL_MS}
  --flamegraph-dir DIR     Brendan Gregg FlameGraph checkout. Default: ${FLAMEGRAPH_DIR}
  --open                   open the SVG after generation.
  -h, --help               show this help.

Environment variables with the same uppercase names may also be used.
EOF
}

extra_pbrt_args=()
while (($#)); do
    case "$1" in
        --pbrt) PBRT_EXE="$2"; shift 2 ;;
        --scene) SCENE="$2"; shift 2 ;;
        --out-dir) OUT_DIR="$2"; shift 2 ;;
        --trace-name) TRACE_NAME="$2"; shift 2 ;;
        --nthreads) NTHREADS="$2"; shift 2 ;;
        --spp) SPP="$2"; shift 2 ;;
        --duration) SAMPLE_DURATION="$2"; shift 2 ;;
        --interval-ms) SAMPLE_INTERVAL_MS="$2"; shift 2 ;;
        --flamegraph-dir) FLAMEGRAPH_DIR="$2"; shift 2 ;;
        --open) OPEN_SVG=1; shift ;;
        -h|--help) usage; exit 0 ;;
        --) shift; extra_pbrt_args=("$@"); break ;;
        *) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
    esac
done

if [[ ! -x "${PBRT_EXE}" ]]; then
    echo "Cannot execute pbrt: ${PBRT_EXE}" >&2
    exit 1
fi
if [[ ! -f "${SCENE}" ]]; then
    echo "Cannot find scene: ${SCENE}" >&2
    exit 1
fi
if [[ ! -f "${FLAMEGRAPH_DIR}/stackcollapse-sample.awk" || ! -f "${FLAMEGRAPH_DIR}/flamegraph.pl" ]]; then
    cat >&2 <<EOF
Cannot find FlameGraph scripts under: ${FLAMEGRAPH_DIR}
Install them with:
  git clone --depth 1 https://github.com/brendangregg/FlameGraph.git "${FLAMEGRAPH_DIR}"
EOF
    exit 1
fi

mkdir -p "${OUT_DIR}"
scene_dir="$(cd -- "$(dirname -- "${SCENE}")" && pwd)"
sample_txt="${OUT_DIR}/${TRACE_NAME}.sample.txt"
collapsed_txt="${OUT_DIR}/${TRACE_NAME}.collapsed.txt"
svg="${OUT_DIR}/${TRACE_NAME}.svg"
pbrt_log="${OUT_DIR}/${TRACE_NAME}.pbrt.log"
outfile="${OUT_DIR}/${TRACE_NAME}.png"

echo "Running pbrt and sampling with /usr/bin/sample..."
echo "  pbrt: ${PBRT_EXE}"
echo "  scene: ${SCENE}"
echo "  sample: ${SAMPLE_DURATION}s @ ${SAMPLE_INTERVAL_MS}ms"

old_pwd="${PWD}"
cd "${scene_dir}"
"${PBRT_EXE}" \
    --nthreads "${NTHREADS}" \
    --spp "${SPP}" \
    --outfile "${outfile}" \
    ${extra_pbrt_args+"${extra_pbrt_args[@]}"} \
    "${SCENE}" \
    >"${pbrt_log}" 2>&1 &
pbrt_pid=$!
cd "${old_pwd}"

cleanup() {
    if kill -0 "${pbrt_pid}" 2>/dev/null; then
        kill "${pbrt_pid}" 2>/dev/null || true
    fi
}
trap cleanup INT TERM

sample_status=0
/usr/bin/sample "${pbrt_pid}" "${SAMPLE_DURATION}" "${SAMPLE_INTERVAL_MS}" -mayDie -file "${sample_txt}" || sample_status=$?

wait "${pbrt_pid}"
trap - INT TERM

if [[ "${sample_status}" -ne 0 ]]; then
    echo "sample failed with exit code ${sample_status}; see ${sample_txt} and ${pbrt_log}" >&2
    exit "${sample_status}"
fi

awk -f "${FLAMEGRAPH_DIR}/stackcollapse-sample.awk" "${sample_txt}" > "${collapsed_txt}"
LC_ALL=C perl "${FLAMEGRAPH_DIR}/flamegraph.pl" \
    --title "pbrt CPU Flame Graph (${TRACE_NAME})" \
    --countname samples \
    "${collapsed_txt}" > "${svg}"

echo "Wrote:"
echo "  ${sample_txt}"
echo "  ${collapsed_txt}"
echo "  ${svg}"
echo "  ${pbrt_log}"

if [[ "${OPEN_SVG}" == "1" ]]; then
    open "${svg}"
fi
