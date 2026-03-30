# coding=utf8
import json
import os
import platform
import shutil
import subprocess
import sys
from datetime import datetime
from pathlib import Path

from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
OUTPUT_DIR = PROJECT_ROOT / "output"
ELF_PATH = OUTPUT_DIR / "main.elf"
MAP_PATH = OUTPUT_DIR / "main.map"
README_PATH = OUTPUT_DIR / "README.md"
RESULT_JSON_PATH = OUTPUT_DIR / "size_results.json"
RESOURCE_DST = OUTPUT_DIR / "app_egui_resource_merge.bin"

BUILD_PORT = "qemu"
BUILD_CPU_ARCH = "cortex-m0plus"
RUNTIME_MACHINE = "mps2-an385"
RUNTIME_CPU = "cortex-m3"
RUNTIME_TIMEOUT = 180

QEMU_MEASURE_CFLAGS = "-DQEMU_HEAP_MEASURE=1 -DQEMU_HEAP_ACTIONS_APP_RECORDING=1 -DEGUI_CONFIG_RECORDING_TEST=1"


class ElfSizeInfo(object):
    def __init__(self):
        self.code_size = 0
        self.rodata_size = 0
        self.data_size = 0
        self.bss_size = 0
        self.bss_pfb_size = 0


def utils_process_elf_file(filename):
    elf_size_info = ElfSizeInfo()

    with open(filename, "rb") as f:
        elffile = ELFFile(f)
        section = elffile.get_section_by_name(".symtab")

        if not section:
            raise RuntimeError("No symbol table found in ELF")

        if isinstance(section, SymbolTableSection):
            for symbol in section.iter_symbols():
                if symbol.name == "__code_size":
                    elf_size_info.code_size = symbol["st_value"]
                elif symbol.name == "__rodata_size":
                    elf_size_info.rodata_size = symbol["st_value"]
                elif symbol.name == "__data_size":
                    elf_size_info.data_size = symbol["st_value"]
                elif symbol.name == "__bss_size":
                    elf_size_info.bss_size = symbol["st_value"]
                elif symbol.name == "__bss_pfb_size":
                    elf_size_info.bss_pfb_size = symbol["st_value"]

    return elf_size_info


def find_qemu_executable():
    exe_name = "qemu-system-arm.exe" if platform.system() == "Windows" else "qemu-system-arm"

    qemu_path = os.environ.get("QEMU_PATH")
    if qemu_path:
        candidate = Path(qemu_path) / exe_name
        if candidate.exists():
            return str(candidate)

    qemu = shutil.which("qemu-system-arm")
    if qemu:
        return qemu

    if platform.system() == "Windows":
        candidates = [
            Path("C:/Program Files/qemu") / exe_name,
            Path("C:/Program Files (x86)/qemu") / exe_name,
            Path(os.environ.get("ProgramFiles", "C:/Program Files")) / "qemu" / exe_name,
            Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "qemu" / exe_name,
        ]
        for candidate in candidates:
            if candidate.exists():
                return str(candidate)

    return "qemu-system-arm"


def discover_cases():
    hello_basic_root = PROJECT_ROOT / "example" / "HelloBasic"
    hello_basic_subs = sorted(
        path.name for path in hello_basic_root.iterdir() if path.is_dir() and (path / "app_egui_config.h").exists()
    )

    cases = []
    for app_sub in hello_basic_subs:
        cases.append(
            {
                "name": "HelloBasic(%s)" % app_sub,
                "app": "HelloBasic",
                "app_sub": app_sub,
            }
        )

    for app in ["HelloSimple", "HelloPerformance", "HelloShowcase", "HelloStyleDemo"]:
        cases.append(
            {
                "name": app,
                "app": app,
            }
        )

    cases.append(
        {
            "name": "HelloVirtual(virtual_stage_showcase)",
            "app": "HelloVirtual",
            "app_sub": "virtual_stage_showcase",
        }
    )
    return cases


def get_case_resource_bin(case):
    if case.get("app_sub"):
        return PROJECT_ROOT / "example" / case["app"] / case["app_sub"] / "resource" / "app_egui_resource_merge.bin"
    return PROJECT_ROOT / "example" / case["app"] / "resource" / "app_egui_resource_merge.bin"


def sync_runtime_resource(case):
    if RESOURCE_DST.exists():
        RESOURCE_DST.unlink()

    resource_src = get_case_resource_bin(case)
    if resource_src.exists():
        RESOURCE_DST.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy(resource_src, RESOURCE_DST)


def get_case_obj_suffix(case, mode):
    safe_name = []
    for ch in case["name"]:
        if ch.isalnum():
            safe_name.append(ch.lower())
        else:
            safe_name.append("_")
    return "size_%s_%s" % ("".join(safe_name).strip("_"), mode)


def format_cmd(cmd):
    return " ".join(cmd)


def run_cmd(cmd, timeout):
    return subprocess.run(
        cmd,
        cwd=PROJECT_ROOT,
        capture_output=True,
        text=True,
        timeout=timeout,
    )


def remove_stale_build_outputs():
    for path in [ELF_PATH, MAP_PATH, OUTPUT_DIR / "main.bin"]:
        if path.exists():
            path.unlink()


def build_case(case, mode, user_cflags=""):
    remove_stale_build_outputs()

    cmd = [
        "make",
        "all",
        "-j",
        "APP=%s" % case["app"],
        "PORT=%s" % BUILD_PORT,
        "CPU_ARCH=%s" % BUILD_CPU_ARCH,
        "APP_OBJ_SUFFIX=%s" % get_case_obj_suffix(case, mode),
    ]

    if case.get("app_sub"):
        cmd.append("APP_SUB=%s" % case["app_sub"])
    if user_cflags:
        cmd.append("USER_CFLAGS=%s" % user_cflags)

    result = run_cmd(cmd, timeout=300)
    return cmd, result


def run_qemu(timeout):
    qemu_cmd = [
        find_qemu_executable(),
        "-machine",
        RUNTIME_MACHINE,
        "-cpu",
        RUNTIME_CPU,
        "-nographic",
        "-semihosting-config",
        "enable=on,target=native",
        "-icount",
        "shift=0",
        "-kernel",
        str(ELF_PATH),
    ]

    proc = subprocess.Popen(
        qemu_cmd,
        cwd=PROJECT_ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )

    try:
        stdout, _ = proc.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        proc.kill()
        stdout, _ = proc.communicate()
        raise RuntimeError("QEMU timeout after %ss\n%s" % (timeout, stdout[-2000:]))

    return stdout


def parse_measure_output(output):
    metrics = {}
    complete = False

    for line in output.splitlines():
        if line.startswith("HEAP_RESULT:"):
            payload = line[len("HEAP_RESULT:") :]
            if "=" not in payload:
                continue
            key, value = payload.split("=", 1)
            metrics[key.strip()] = int(value.strip())
        elif "HEAP_EXIT" in line:
            complete = True

    required = [
        "idle_current",
        "idle_peak",
        "interaction_action_count",
        "interaction_delta_peak",
        "interaction_total_peak",
        "stack_peak_bytes",
    ]
    missing = [key for key in required if key not in metrics]
    if missing:
        raise RuntimeError("Missing metrics: %s\n%s" % (", ".join(missing), output[-2000:]))
    if not complete:
        raise RuntimeError("HEAP_EXIT not found\n%s" % output[-2000:])

    return metrics


def make_failure(case, phase, message, command=None):
    item = {
        "name": case["name"],
        "phase": phase,
        "message": message,
    }
    if command is not None:
        item["command"] = command
    return item


def process_case(current_work_cnt, total_work_cnt, case):
    print("=================================================================================")
    print(
        "Total Work Cnt: %d, Current Cnt: %d, Process: %.2f%%, Case: %s"
        % (total_work_cnt, current_work_cnt, current_work_cnt * 100.0 / total_work_cnt, case["name"])
    )
    print("=================================================================================")

    size_cmd, size_result = build_case(case, "size")
    print(format_cmd(size_cmd))
    if size_result.returncode != 0:
        message = (size_result.stderr or size_result.stdout)[-4000:]
        return None, make_failure(case, "build_size", message, format_cmd(size_cmd))

    if not ELF_PATH.exists():
        return None, make_failure(case, "build_size", "ELF not found after size build", format_cmd(size_cmd))

    elf_size_info = utils_process_elf_file(str(ELF_PATH))

    measure_cmd, measure_result = build_case(case, "measure", QEMU_MEASURE_CFLAGS)
    print(format_cmd(measure_cmd))
    if measure_result.returncode != 0:
        message = (measure_result.stderr or measure_result.stdout)[-4000:]
        return None, make_failure(case, "build_measure", message, format_cmd(measure_cmd))

    sync_runtime_resource(case)

    try:
        qemu_output = run_qemu(RUNTIME_TIMEOUT)
        metrics = parse_measure_output(qemu_output)
    except Exception as exc:
        return None, make_failure(case, "run_measure", str(exc))

    heap_peak_bytes = metrics["idle_peak"]
    if metrics["interaction_total_peak"] > heap_peak_bytes:
        heap_peak_bytes = metrics["interaction_total_peak"]

    json_entry = {
        "name": case["name"],
        "app": case["app"],
        "app_sub": case.get("app_sub"),
        "code_bytes": elf_size_info.code_size,
        "resource_bytes": elf_size_info.rodata_size,
        "ram_bytes": elf_size_info.data_size + elf_size_info.bss_size - elf_size_info.bss_pfb_size,
        "pfb_bytes": elf_size_info.bss_pfb_size,
        "heap_idle_bytes": metrics["idle_current"],
        "heap_init_peak_bytes": metrics["idle_peak"],
        "heap_interaction_peak_bytes": metrics["interaction_total_peak"],
        "heap_peak_bytes": heap_peak_bytes,
        "stack_peak_bytes": metrics["stack_peak_bytes"],
        "interaction_action_count": metrics["interaction_action_count"],
    }
    return json_entry, None


def build_readme(size_results, failures):
    lines = []
    lines.append("# QEMU Size Report")
    lines.append("")
    lines.append("- Build target: `PORT=qemu CPU_ARCH=%s`" % BUILD_CPU_ARCH)
    lines.append("- Runtime measurement: `qemu-system-arm -machine %s -cpu %s`" % (RUNTIME_MACHINE, RUNTIME_CPU))
    lines.append("- Scope: `HelloBasic/*`, `HelloSimple`, `HelloPerformance`, `HelloShowcase`, `HelloStyleDemo`, `HelloVirtual(virtual_stage_showcase)`")
    lines.append("")
    lines.append("| App | Code(Bytes) | Resource(Bytes) | RAM(Bytes) | PFB(Bytes) | Heap Peak(Bytes) | Stack Peak(Bytes) |")
    lines.append("|-----|-------------|-----------------|------------|------------|------------------|-------------------|")

    for item in size_results:
        lines.append(
            "| %s | %d | %d | %d | %d | %d | %d |"
            % (
                item["name"],
                item["code_bytes"],
                item["resource_bytes"],
                item["ram_bytes"],
                item["pfb_bytes"],
                item["heap_peak_bytes"],
                item["stack_peak_bytes"],
            )
        )

    if failures:
        lines.append("")
        lines.append("## Failures")
        lines.append("")
        for failure in failures:
            lines.append("- `%s` [%s]: %s" % (failure["name"], failure["phase"], failure["message"].replace("\n", " ")))

    return "\n".join(lines) + "\n"


def get_git_commit():
    try:
        return subprocess.check_output(
            ["git", "rev-parse", "--short", "HEAD"],
            cwd=PROJECT_ROOT,
            stderr=subprocess.DEVNULL,
        ).decode().strip()
    except Exception:
        return "unknown"


def main():
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    cases = discover_cases()
    total_work_cnt = len(cases)

    size_results = []
    failures = []
    for index, case in enumerate(cases, 1):
        json_entry, failure = process_case(index, total_work_cnt, case)
        if json_entry is not None:
            size_results.append(json_entry)
        if failure is not None:
            failures.append(failure)

    README_PATH.write_text(build_readme(size_results, failures), encoding="utf-8")
    print("Markdown saved: %s" % README_PATH)

    json_data = {
        "timestamp": datetime.now().isoformat(),
        "git_commit": get_git_commit(),
        "build_platform": {
            "port": BUILD_PORT,
            "cpu_arch": BUILD_CPU_ARCH,
        },
        "runtime_platform": {
            "machine": RUNTIME_MACHINE,
            "cpu": RUNTIME_CPU,
        },
        "scope": {
            "hello_basic_all_subcases": True,
            "apps": [
                "HelloSimple",
                "HelloPerformance",
                "HelloShowcase",
                "HelloStyleDemo",
                "HelloVirtual(virtual_stage_showcase)",
            ],
        },
        "apps": size_results,
        "failures": failures,
    }
    RESULT_JSON_PATH.write_text(json.dumps(json_data, indent=2, ensure_ascii=False), encoding="utf-8")
    print("JSON saved: %s" % RESULT_JSON_PATH)

    if failures:
        print("Completed with %d failures." % len(failures))
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
