#!/usr/bin/env python3
"""
Collect an Android simpleperf profile for com.Blurredcode.TP_ThirdPerson and
turn it into reports plus a FlameGraph SVG when report-sample output is
available.
"""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path


DEFAULT_PACKAGE = "com.Blurredcode.TP_ThirdPerson"
DEFAULT_TRACE_NAME = "tp-thirdperson-simpleperf"


@dataclass
class Sample:
    tid: int | None = None
    thread_name: str = "unknown"
    period: int = 1
    frames: list[str] = field(default_factory=list)


def script_dir() -> Path:
    return Path(__file__).resolve().parent


def sh_quote(value: str) -> str:
    return "'" + value.replace("'", "'\\''") + "'"


def run(args: list[str], *, stdout=None, check: bool = True) -> subprocess.CompletedProcess:
    print("+ " + " ".join(args))
    completed = subprocess.run(args, stdout=stdout, text=stdout is not None)
    if check and completed.returncode != 0:
        raise RuntimeError(f"Command failed with exit code {completed.returncode}: {' '.join(args)}")
    return completed


def adb_shell(adb: str, command: str, *, check: bool = True) -> subprocess.CompletedProcess:
    return run([adb, "shell", command], check=check)


def adb_shell_capture(adb: str, command: str, *, check: bool = True) -> str:
    print("+ " + " ".join([adb, "shell", command]))
    completed = subprocess.run([adb, "shell", command], capture_output=True, text=True)
    if check and completed.returncode != 0:
        raise RuntimeError(
            f"Command failed with exit code {completed.returncode}: {adb} shell {command}\n"
            f"{completed.stderr.strip()}"
        )
    return completed.stdout.strip()


def resolve_perl(requested: str | None) -> str | None:
    candidates: list[str] = []
    if requested:
        candidates.append(requested)
    path_perl = shutil.which("perl")
    if path_perl:
        candidates.append(path_perl)
    candidates.extend(
        [
            r"C:\Program Files\Git\usr\bin\perl.exe",
            r"C:\Strawberry\perl\bin\perl.exe",
            r"C:\msys64\usr\bin\perl.exe",
            r"D:\msys64\usr\bin\perl.exe",
        ]
    )
    for candidate in candidates:
        path = Path(candidate)
        if path.is_file():
            return str(path)
    return None


def try_pull(adb: str, remote_path: str, local_path: Path) -> bool:
    completed = run([adb, "pull", remote_path, str(local_path)], check=False)
    return completed.returncode == 0 and local_path.exists()


def safe_filename(value: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9_.-]+", "_", value.strip())
    return cleaned.strip("._") or "unknown"


def build_report_command(remote_data: str, args: argparse.Namespace, kind: str, remote_out: str) -> str:
    include_tid = f" --include-tid {args.include_tid}" if args.include_tid else ""
    if kind == "symbols":
        report_args = f"--sort dso,symbol -n --percent-limit {args.symbol_percent_limit}"
    elif kind == "dso":
        report_args = "--sort dso -n --percent-limit 0"
    elif kind == "callgraph":
        report_args = (
            f"--children -g caller --full-callgraph --max-stack {args.max_stack} "
            f"--percent-limit {args.callgraph_percent_limit}"
        )
    else:
        raise ValueError(kind)
    return f"simpleperf report -i {sh_quote(remote_data)}{include_tid} {report_args} -o {sh_quote(remote_out)}"


def clean_frame(line: str) -> str | None:
    text = line.strip()
    if not text:
        return None
    if re.match(r"^(sample|event|time|period|pid|tid|thread|comm)\b", text, re.I):
        return None
    text = re.sub(r"^(?:#\d+\s+)?(?:0x[0-9a-fA-F]+|\[0x[0-9a-fA-F]+\])\s+", "", text)
    text = re.sub(r"^\d+\s+", "", text)
    text = re.sub(r"\s+", " ", text).strip()
    if not text or text == "callchain:":
        return None
    return text.replace(";", ":")


def clean_symbol(value: str) -> str | None:
    text = value.strip()
    if not text:
        return None
    return text.replace(";", ":")


def read_report_samples(sample_text: Path) -> list[Sample]:
    samples: list[Sample] = []
    current: Sample | None = None
    in_callchain = False
    pending_callchain_symbol = False

    def flush() -> None:
        nonlocal current, in_callchain, pending_callchain_symbol
        if current and current.frames:
            samples.append(current)
        current = None
        in_callchain = False
        pending_callchain_symbol = False

    for raw_line in sample_text.read_text(encoding="utf-8", errors="replace").splitlines():
        stripped = raw_line.strip()
        if not stripped:
            continue
        if stripped == "sample:":
            flush()
            current = Sample()
            continue
        if current is None:
            continue

        key_match = re.match(r"^([A-Za-z_]+):\s*(.*)$", stripped)
        if not key_match:
            continue
        key = key_match.group(1)
        value = key_match.group(2)

        if key == "thread_id":
            try:
                current.tid = int(value)
            except ValueError:
                current.tid = None
        elif key == "thread_name":
            current.thread_name = value or "unknown"
        elif key in ("event_count", "period", "sample_period"):
            try:
                current.period = max(int(value), 1)
            except ValueError:
                current.period = 1
        elif key == "callchain":
            in_callchain = True
            pending_callchain_symbol = False
        elif key == "symbol":
            symbol = clean_symbol(value)
            if not symbol:
                continue
            if in_callchain or pending_callchain_symbol:
                current.frames.append(symbol)
                pending_callchain_symbol = False
            elif not current.frames:
                current.frames.append(symbol)
        elif in_callchain and key == "vaddr_in_file":
            pending_callchain_symbol = True

    flush()
    return samples


def write_collapsed_stacks(
    samples: list[Sample],
    collapsed_text: Path,
    *,
    reverse_callchain: bool,
    tid_filter: int | None = None,
) -> tuple[int, int]:
    stacks: dict[str, int] = {}
    sample_count = 0
    for sample in samples:
        if tid_filter is not None and sample.tid != tid_filter:
            continue
        if not sample.frames:
            continue
        stack_frames = list(reversed(sample.frames)) if reverse_callchain else sample.frames
        stack = ";".join(stack_frames)
        stacks[stack] = stacks.get(stack, 0) + max(sample.period, 1)
        sample_count += 1

    with collapsed_text.open("w", encoding="utf-8", newline="\n") as out_file:
        for stack, count in sorted(stacks.items()):
            out_file.write(f"{stack} {count}\n")
    return len(stacks), sample_count


def write_thread_summary(samples: list[Sample], summary_path: Path) -> list[dict[str, object]]:
    threads: dict[int, dict[str, object]] = {}
    for sample in samples:
        if sample.tid is None:
            continue
        info = threads.setdefault(
            sample.tid,
            {"tid": sample.tid, "name": sample.thread_name, "samples": 0, "event_count": 0},
        )
        info["samples"] = int(info["samples"]) + 1
        info["event_count"] = int(info["event_count"]) + max(sample.period, 1)
        if sample.thread_name and sample.thread_name != "unknown":
            info["name"] = sample.thread_name

    ranked = sorted(
        threads.values(),
        key=lambda item: (int(item["event_count"]), int(item["samples"])),
        reverse=True,
    )
    total_event_count = sum(int(item["event_count"]) for item in ranked)
    with summary_path.open("w", encoding="utf-8", newline="\n") as out_file:
        out_file.write("Rank\tTID\tThreadName\tSamples\tEventCount\tPercent\n")
        for index, item in enumerate(ranked, 1):
            percent = (int(item["event_count"]) / total_event_count * 100.0) if total_event_count else 0.0
            out_file.write(
                f"{index}\t{item['tid']}\t{item['name']}\t{item['samples']}\t"
                f"{item['event_count']}\t{percent:.2f}%\n"
            )
    return ranked


def parse_simpleperf_report_sample(sample_text: Path, collapsed_text: Path, *, reverse_callchain: bool) -> int:
    samples = read_report_samples(sample_text)
    stack_count, _ = write_collapsed_stacks(samples, collapsed_text, reverse_callchain=reverse_callchain)
    return stack_count


def generate_flamegraph(perl: str, flamegraph_pl: Path, collapsed_text: Path, svg: Path, title: str) -> None:
    with svg.open("w", encoding="utf-8", newline="\n") as svg_file:
        run(
            [
                perl,
                str(flamegraph_pl),
                "--title",
                title,
                "--countname",
                "events",
                str(collapsed_text),
            ],
            stdout=svg_file,
        )


def generate_top_thread_flamegraphs(
    sample_text: Path,
    out_dir: Path,
    args: argparse.Namespace,
    perl: str,
    flamegraph_pl: Path,
) -> list[Path]:
    samples = read_report_samples(sample_text)
    summary_path = out_dir / f"{args.trace_name}_threads.txt"
    ranked_threads = write_thread_summary(samples, summary_path)
    selected = [
        item
        for item in ranked_threads
        if int(item["samples"]) >= args.min_thread_samples and int(item["event_count"]) > 0
    ][: args.top_threads]

    thread_dir = out_dir / f"{args.trace_name}_threads"
    thread_dir.mkdir(parents=True, exist_ok=True)
    generated: list[Path] = [summary_path]
    for rank, item in enumerate(selected, 1):
        tid = int(item["tid"])
        thread_name = str(item["name"])
        prefix = f"{rank:02d}_tid{tid}_{safe_filename(thread_name)}"
        collapsed_text = thread_dir / f"{prefix}.collapsed.txt"
        svg = thread_dir / f"{prefix}.svg"
        stack_count, sample_count = write_collapsed_stacks(
            samples,
            collapsed_text,
            reverse_callchain=not args.no_reverse_callchain,
            tid_filter=tid,
        )
        if stack_count == 0 or sample_count == 0:
            continue
        title = f"Android simpleperf Thread Flame Graph ({thread_name}, tid {tid})"
        generate_flamegraph(perl, flamegraph_pl, collapsed_text, svg, title)
        generated.extend([collapsed_text, svg])
    return generated



def generate_sample_text(args: argparse.Namespace, remote_data: str, local_data: Path, sample_text: Path) -> bool:
    remote_sample = f"{args.remote_dir.rstrip('/')}/{args.trace_name}.samples.txt"
    device_command = (
        f"simpleperf report-sample -i {sh_quote(remote_data)} --show-callchain "
        f"> {sh_quote(remote_sample)}"
    )
    if adb_shell(args.adb, device_command, check=False).returncode == 0:
        return try_pull(args.adb, remote_sample, sample_text)

    host_simpleperf = args.simpleperf or shutil.which("simpleperf")
    if not host_simpleperf:
        print("simpleperf report-sample is not available on the device, and host simpleperf was not found.")
        return False

    with sample_text.open("w", encoding="utf-8", newline="\n") as out_file:
        completed = run(
            [host_simpleperf, "report-sample", "-i", str(local_data), "--show-callchain"],
            stdout=out_file,
            check=False,
        )
    return completed.returncode == 0 and sample_text.stat().st_size > 0


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Collect Android simpleperf data and generate reports/flame graph for an app."
    )
    parser.add_argument("--package", default=DEFAULT_PACKAGE, help=f"Android package name. Default: {DEFAULT_PACKAGE}")
    parser.add_argument("--out-dir", default=str(script_dir() / "out-android"), help="Local output directory.")
    parser.add_argument("--trace-name", default=DEFAULT_TRACE_NAME, help="Output file prefix.")
    parser.add_argument("--remote-dir", default="/data/local/tmp", help="Remote directory for simpleperf outputs.")
    parser.add_argument("--duration", type=int, default=5, help="Record duration in seconds.")
    parser.add_argument("--frequency", "-f", type=int, default=99, help="Sampling frequency.")
    parser.add_argument("--event", default="task-clock:u", help="simpleperf event.")
    parser.add_argument("--call-graph", default="fp", choices=["fp", "dwarf"], help="Call graph mode.")
    parser.add_argument("--include-tid", type=int, help="Restrict reports to a specific thread id.")
    parser.add_argument("--max-stack", type=int, default=30, help="Max stack depth for text callgraph report.")
    parser.add_argument("--symbol-percent-limit", default="0.1", help="Percent limit for symbol report.")
    parser.add_argument("--callgraph-percent-limit", default="0.5", help="Percent limit for callgraph report.")
    parser.add_argument("--adb", default="adb", help="adb executable.")
    parser.add_argument("--simpleperf", help="Host simpleperf executable for report-sample fallback.")
    parser.add_argument("--perl", help="perl executable used by FlameGraph/flamegraph.pl.")
    parser.add_argument("--flamegraph-dir", default=str(script_dir() / "FlameGraph"), help="FlameGraph checkout path.")
    parser.add_argument("--skip-record", action="store_true", help="Use existing remote perf.data instead of recording.")
    parser.add_argument("--no-svg", action="store_true", help="Only collect perf.data and text reports.")
    parser.add_argument("--split-threads", action="store_true", help="Generate per-thread flame graphs from top active threads.")
    parser.add_argument("--top-threads", type=int, default=5, help="Number of busiest threads to keep with --split-threads.")
    parser.add_argument(
        "--min-thread-samples",
        type=int,
        default=2,
        help="Drop threads with fewer samples when generating per-thread flame graphs.",
    )
    parser.add_argument("--keep-remote", action="store_true", help="Do not delete remote output files.")
    parser.add_argument("--no-reverse-callchain", action="store_true", help="Keep report-sample callchain order as-is.")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    remote_base = f"{args.remote_dir.rstrip('/')}/{args.trace_name}"
    remote_data = f"{remote_base}.data"
    local_data = out_dir / f"{args.trace_name}.data"
    local_symbols = out_dir / f"{args.trace_name}_symbols.txt"
    local_dso = out_dir / f"{args.trace_name}_dso.txt"
    local_callgraph = out_dir / f"{args.trace_name}_callgraph.txt"
    sample_text = out_dir / f"{args.trace_name}.samples.txt"
    collapsed_text = out_dir / f"{args.trace_name}.collapsed.txt"
    svg = out_dir / f"{args.trace_name}.svg"
    generated_thread_paths: list[Path] = []

    print(f"Package: {args.package}")
    print(f"Output:  {out_dir}")

    run([args.adb, "get-state"])
    pid = adb_shell_capture(args.adb, f"pidof {sh_quote(args.package)}", check=False)
    if pid:
        print(f"Running PID: {pid}")
    else:
        print("Warning: package PID was not found. Start the app before recording.")

    adb_shell(args.adb, "which simpleperf || ls /system/bin/simpleperf /system/xbin/simpleperf 2>/dev/null")

    remote_paths = [
        remote_data,
        f"{remote_base}_symbols.txt",
        f"{remote_base}_dso.txt",
        f"{remote_base}_callgraph.txt",
        f"{remote_base}.samples.txt",
    ]

    if not args.skip_record:
        record_command = (
            f"simpleperf record --app {sh_quote(args.package)} "
            f"-e {sh_quote(args.event)} -f {args.frequency} --duration {args.duration} "
            f"--call-graph {args.call_graph} -o {sh_quote(remote_data)}"
        )
        adb_shell(args.adb, record_command)

    reports = {
        "symbols": (f"{remote_base}_symbols.txt", local_symbols),
        "dso": (f"{remote_base}_dso.txt", local_dso),
        "callgraph": (f"{remote_base}_callgraph.txt", local_callgraph),
    }
    for kind, (remote_report, _) in reports.items():
        adb_shell(args.adb, build_report_command(remote_data, args, kind, remote_report), check=False)

    if not try_pull(args.adb, remote_data, local_data):
        raise RuntimeError(f"Failed to pull {remote_data}")
    for _, (remote_report, local_report) in reports.items():
        try_pull(args.adb, remote_report, local_report)

    if not args.no_svg:
        flamegraph_dir = Path(args.flamegraph_dir).resolve()
        flamegraph_pl = flamegraph_dir / "flamegraph.pl"
        perl = resolve_perl(args.perl)
        if not flamegraph_pl.is_file():
            print(f"Cannot find FlameGraph script: {flamegraph_pl}")
        elif not perl:
            print("Cannot find perl; skipping SVG generation.")
        elif generate_sample_text(args, remote_data, local_data, sample_text):
            stack_count = parse_simpleperf_report_sample(
                sample_text,
                collapsed_text,
                reverse_callchain=not args.no_reverse_callchain,
            )
            if stack_count == 0:
                print(f"No stacks parsed from {sample_text}; skipping SVG generation.")
            else:
                generate_flamegraph(
                    perl,
                    flamegraph_pl,
                    collapsed_text,
                    svg,
                    f"Android simpleperf Flame Graph ({args.package})",
                )
                if args.split_threads:
                    generated_thread_paths = generate_top_thread_flamegraphs(
                        sample_text,
                        out_dir,
                        args,
                        perl,
                        flamegraph_pl,
                    )
        else:
            print("Could not export simpleperf samples; collected perf.data and text reports only.")

    if not args.keep_remote:
        adb_shell(args.adb, "rm -f " + " ".join(sh_quote(path) for path in remote_paths), check=False)

    print("Wrote:")
    for path in [local_data, local_symbols, local_dso, local_callgraph, sample_text, collapsed_text, svg]:
        if path.exists():
            print(f"  {path}")
    for path in generated_thread_paths:
        if path.exists():
            print(f"  {path}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main(sys.argv[1:]))
    except KeyboardInterrupt:
        raise SystemExit(130)
    except RuntimeError as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
