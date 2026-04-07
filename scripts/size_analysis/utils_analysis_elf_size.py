# coding=utf8
import concurrent.futures
import hashlib
import json
import os
import platform
import re
import shutil
import subprocess
import sys
from datetime import datetime
from pathlib import Path

# Import size_to_doc for automatic documentation generation
try:
    import size_to_doc
except ImportError:
    size_to_doc = None


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent
OUTPUT_DIR = PROJECT_ROOT / "output"
ELF_PATH = OUTPUT_DIR / "main.elf"
MAP_PATH = OUTPUT_DIR / "main.map"
README_PATH = OUTPUT_DIR / "README.md"
RESULT_JSON_PATH = OUTPUT_DIR / "size_results.json"
RESOURCE_DST = OUTPUT_DIR / "app_egui_resource_merge.bin"
CASE_BUILD_ROOT = Path("output") / "size_cases"

BUILD_PORT = "qemu"
BUILD_CPU_ARCH = "cortex-m0plus"
RUNTIME_MACHINE = "mps2-an385"
RUNTIME_CPU = "cortex-m3"
DEFAULT_RUNTIME_TIMEOUT_FULL = 180
DEFAULT_RUNTIME_TIMEOUT_TYPICAL = 60

QEMU_MEASURE_CFLAGS = "-DQEMU_HEAP_MEASURE=1 -DQEMU_HEAP_ACTIONS_APP_RECORDING=1 -DEGUI_CONFIG_RECORDING_TEST=1"

DEFAULT_CASE_SET = "typical"
TYPICAL_HELLO_BASIC_SUBS = ("button", "image", "label")
FULL_STANDALONE_APPS = ("HelloSimple", "HelloPerformance", "HelloShowcase", "HelloStyleDemo")
TYPICAL_STANDALONE_APPS = ("HelloSimple", "HelloShowcase")


class ElfSizeInfo(object):
    def __init__(self):
        self.code_size = 0
        self.rodata_size = 0
        self.data_size = 0
        self.bss_size = 0
        self.bss_pfb_size = 0


def create_case(app, app_sub=None):
    case = {
        "name": "%s(%s)" % (app, app_sub) if app_sub else app,
        "app": app,
    }
    if app_sub:
        case["app_sub"] = app_sub
    return case


def normalize_user_cflags(user_cflags):
    return " ".join((user_cflags or "").split())


def get_config_paths(app, app_sub=None):
    config_paths = []
    if app_sub:
        config_paths.append(PROJECT_ROOT / "example" / app / app_sub / "app_egui_config.h")
    config_paths.append(PROJECT_ROOT / "example" / app / "app_egui_config.h")
    return config_paths


def get_config_path(app, app_sub=None):
    for config_path in get_config_paths(app, app_sub):
        if config_path.exists():
            return config_path
    return None


def get_config_hash(app, app_sub=None):
    config_path = get_config_path(app, app_sub)
    if config_path is None:
        return "default"
    return hashlib.sha256(config_path.read_bytes()).hexdigest()[:12]


def utils_process_elf_file(filename):
    raise RuntimeError("ELF symbol size parsing has been replaced by linker map parsing")


MAP_MEMORY_MARKER = "Linker script and memory map"
MAP_TOP_LEVEL_SECTION_RE = re.compile(r"^([.][^\s]*)\s+0x[0-9a-fA-F]+\s+0x[0-9a-fA-F]+(?:\s+load address\s+0x[0-9a-fA-F]+)?$")
MAP_INPUT_SECTION_INLINE_RE = re.compile(r"^\s+([.*][^\s]*|[.][^\s]*)\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+(\S.*)$")
MAP_INPUT_SECTION_NAME_RE = re.compile(r"^\s+([.][^\s]*)\s*$")
MAP_INPUT_SECTION_CONT_RE = re.compile(r"^\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+(\S.*)$")
MAP_OUTPUT_SECTION_TO_FIELD = {
    ".text": "code_size",
    ".rodata": "rodata_size",
    ".data": "data_size",
    ".bss": "bss_size",
}


def normalize_map_object_ref(object_ref):
    return object_ref.strip().replace("\\", "/")


def extract_repo_object_path(object_ref):
    object_ref = normalize_map_object_ref(object_ref)
    match = re.search(r"(?:^|/)output/obj/[^/]+/(.+)$", object_ref)
    if match is None:
        return None
    return match.group(1)


def should_count_repo_object(repo_object_path):
    return repo_object_path.startswith("src/") or repo_object_path.startswith("example/")


def is_matching_output_input_section(output_section, input_section):
    return input_section == output_section or input_section.startswith(output_section + ".")


def collect_map_section_size(size_info, output_section, input_section, object_ref, size):
    repo_object_path = extract_repo_object_path(object_ref)

    if output_section == ".bss" and input_section.startswith(".bss.pfb_area"):
        size_info.bss_pfb_size += size
        return

    if repo_object_path is None or not should_count_repo_object(repo_object_path):
        return

    if not is_matching_output_input_section(output_section, input_section):
        return

    field_name = MAP_OUTPUT_SECTION_TO_FIELD[output_section]
    setattr(size_info, field_name, getattr(size_info, field_name) + size)


def utils_process_map_file(filename):
    size_info = ElfSizeInfo()
    in_memory_map = False
    current_output_section = None
    pending_input_section = None

    with open(filename, "r", encoding="utf-8", errors="ignore") as f:
        for raw_line in f:
            line = raw_line.rstrip("\r\n")

            if not in_memory_map:
                if line.strip() == MAP_MEMORY_MARKER:
                    in_memory_map = True
                continue

            top_level_match = MAP_TOP_LEVEL_SECTION_RE.match(line)
            if top_level_match:
                section_name = top_level_match.group(1)
                current_output_section = section_name if section_name in MAP_OUTPUT_SECTION_TO_FIELD else None
                pending_input_section = None
                continue

            if current_output_section is None:
                pending_input_section = None
                continue

            inline_match = MAP_INPUT_SECTION_INLINE_RE.match(line)
            if inline_match:
                input_section = inline_match.group(1)
                size = int(inline_match.group(3), 16)
                object_ref = inline_match.group(4)
                if size > 0:
                    collect_map_section_size(size_info, current_output_section, input_section, object_ref, size)
                pending_input_section = None
                continue

            name_match = MAP_INPUT_SECTION_NAME_RE.match(line)
            if name_match:
                pending_input_section = name_match.group(1)
                continue

            if pending_input_section is not None:
                cont_match = MAP_INPUT_SECTION_CONT_RE.match(line)
                if cont_match:
                    size = int(cont_match.group(2), 16)
                    object_ref = cont_match.group(3)
                    if size > 0:
                        collect_map_section_size(size_info, current_output_section, pending_input_section, object_ref, size)
                pending_input_section = None
                continue

            pending_input_section = None

    if not in_memory_map:
        raise RuntimeError("Linker memory map section not found: %s" % filename)

    return size_info


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


def discover_cases(case_set):
    hello_basic_root = PROJECT_ROOT / "example" / "HelloBasic"
    hello_basic_subs = sorted(
        path.name for path in hello_basic_root.iterdir() if path.is_dir() and (path / "app_egui_config.h").exists()
    )

    cases = []
    if case_set == "full":
        hello_basic_targets = hello_basic_subs
        standalone_targets = FULL_STANDALONE_APPS
    else:
        hello_basic_targets = [app_sub for app_sub in TYPICAL_HELLO_BASIC_SUBS if app_sub in hello_basic_subs]
        standalone_targets = TYPICAL_STANDALONE_APPS

    for app_sub in hello_basic_targets:
        cases.append(create_case("HelloBasic", app_sub))

    for app in standalone_targets:
        cases.append(create_case(app))

    cases.append(create_case("HelloVirtual", "virtual_stage_showcase"))
    return cases


def describe_scope(case_set):
    if case_set == "full":
        return "`HelloBasic/*`, `HelloSimple`, `HelloPerformance`, `HelloShowcase`, `HelloStyleDemo`, `HelloVirtual(virtual_stage_showcase)`"
    return "`HelloBasic(button,image,label)`, `HelloSimple`, `HelloShowcase`, `HelloVirtual(virtual_stage_showcase)`"


def describe_static_size_scope():
    return "Map input sections from repo-side `src/` + `example/` objects only; exclude toolchain libraries, `driver/`, `porting/`; `ram_bytes` excludes `PFB`, and `pfb_bytes` still comes from each case-local `main.map` `.bss.pfb_area`."


def get_case_resource_bin(case):
    if case.get("app_sub"):
        return PROJECT_ROOT / "example" / case["app"] / case["app_sub"] / "resource" / "app_egui_resource_merge.bin"
    return PROJECT_ROOT / "example" / case["app"] / "resource" / "app_egui_resource_merge.bin"


def sync_runtime_resource(case, build_output_dir):
    resource_dst = Path(build_output_dir) / "app_egui_resource_merge.bin"
    if resource_dst.exists():
        resource_dst.unlink()

    resource_src = get_case_resource_bin(case)
    if resource_src.exists():
        resource_dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy(resource_src, resource_dst)

def get_case_obj_suffix(case, mode, user_cflags="", share_objects=False):
    safe_name = []
    for ch in case["name"]:
        if ch.isalnum():
            safe_name.append(ch.lower())
        else:
            safe_name.append("_")
    if not share_objects or case["app"] not in ("HelloBasic", "HelloVirtual", "HelloCustomWidgets", "HelloSizeAnalysis"):
        return "size_%s_%s" % ("".join(safe_name).strip("_"), mode)

    scope_signature = hashlib.sha1(
        json.dumps(
            {
                "mode": mode,
                "user_cflags": normalize_user_cflags(user_cflags),
            },
            sort_keys=True,
            separators=(",", ":"),
        ).encode("utf-8")
    ).hexdigest()[:8]
    return "size_%s_cfg_%s_%s" % (case["app"].lower(), get_config_hash(case["app"], case.get("app_sub")), scope_signature)


def get_case_build_output_dir(case, mode, user_cflags=""):
    payload = {
        "name": case["name"],
        "mode": mode,
        "user_cflags": normalize_user_cflags(user_cflags),
    }
    signature = hashlib.sha1(json.dumps(payload, sort_keys=True, separators=(",", ":")).encode("utf-8")).hexdigest()[:10]
    return CASE_BUILD_ROOT / signature


def format_cmd(cmd):
    return " ".join(cmd)


def run_cmd(cmd, timeout, cwd=None):
    return subprocess.run(
        cmd,
        cwd=cwd or PROJECT_ROOT,
        capture_output=True,
        text=True,
        timeout=timeout,
    )


def remove_stale_build_outputs(build_output_dir):
    build_output_dir = Path(build_output_dir)
    for path in [build_output_dir / "main.elf", build_output_dir / "main.map", build_output_dir / "main.bin"]:
        if path.exists():
            path.unlink()


def build_case(case, mode, user_cflags="", build_output_dir=None, app_obj_suffix=None, objroot_path=None, share_objects=False):
    if build_output_dir is None:
        build_output_dir = get_case_build_output_dir(case, mode, user_cflags=user_cflags)
    if app_obj_suffix is None:
        app_obj_suffix = get_case_obj_suffix(case, mode, user_cflags=user_cflags, share_objects=share_objects)

    remove_stale_build_outputs(build_output_dir)

    # Use all CPU cores for parallel compilation
    import multiprocessing
    num_cores = multiprocessing.cpu_count()

    cmd = [
        "make",
        "all",
        "-j%d" % num_cores,
        "APP=%s" % case["app"],
        "PORT=%s" % BUILD_PORT,
        "CPU_ARCH=%s" % BUILD_CPU_ARCH,
        "APP_OBJ_SUFFIX=%s" % app_obj_suffix,
        "OUTPUT_PATH=%s" % Path(build_output_dir).as_posix(),
    ]
    if objroot_path is not None:
        cmd.append("OBJROOT_PATH=%s" % Path(objroot_path).as_posix())

    if case.get("app_sub"):
        cmd.append("APP_SUB=%s" % case["app_sub"])
    if user_cflags:
        cmd.append("USER_CFLAGS=%s" % user_cflags)

    result = run_cmd(cmd, timeout=300)
    return cmd, result


def run_qemu(build_output_dir, timeout):
    build_output_dir = Path(build_output_dir)
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
        "main.elf",
    ]

    proc = subprocess.Popen(
        qemu_cmd,
        cwd=build_output_dir,
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


def get_auto_parallel_jobs(total_cases):
    if total_cases <= 1:
        return 1
    cpu_count = os.cpu_count() or 1
    return max(1, min(total_cases, 4, cpu_count // 4 or 1))


def resolve_parallel_jobs(requested_jobs, total_cases):
    if requested_jobs < 0:
        raise ValueError("--jobs must be >= 0")
    if total_cases <= 1:
        return 1
    if requested_jobs > 0:
        return max(1, min(total_cases, requested_jobs))
    return get_auto_parallel_jobs(total_cases)


def process_cases_parallel(cases, runtime_timeout, jobs):
    total_work_cnt = len(cases)
    if total_work_cnt == 0:
        return [], []

    parallel_jobs = resolve_parallel_jobs(jobs, total_work_cnt)
    if parallel_jobs <= 1:
        size_results = []
        failures = []
        for index, case in enumerate(cases, 1):
            json_entry, failure = process_case(index, total_work_cnt, case, runtime_timeout)
            if json_entry is not None:
                size_results.append(json_entry)
            if failure is not None:
                failures.append(failure)
        return size_results, failures

    print("Running %d size-analysis cases with jobs=%d" % (total_work_cnt, parallel_jobs))
    objroot_path = Path("output")
    grouped = {}
    direct_tasks = []
    for case in cases:
        shared_suffix = get_case_obj_suffix(case, "measure", QEMU_MEASURE_CFLAGS, share_objects=True)
        case_info = {
            "case": case,
            "shared_suffix": shared_suffix,
            "build_output_dir": get_case_build_output_dir(case, "measure", user_cflags=QEMU_MEASURE_CFLAGS),
        }
        if case["app"] in ("HelloBasic", "HelloVirtual", "HelloCustomWidgets", "HelloSizeAnalysis"):
            grouped.setdefault(shared_suffix, []).append(case_info)
        else:
            direct_tasks.append(case_info)

    size_results = []
    failures = []
    completed = 0
    parallel_tasks = list(direct_tasks)

    for shared_suffix in sorted(grouped):
        family_cases = grouped[shared_suffix]
        seed = family_cases[0]
        completed += 1
        json_entry, failure = process_case(
            completed,
            total_work_cnt,
            seed["case"],
            runtime_timeout,
            user_cflags=QEMU_MEASURE_CFLAGS,
            build_output_dir=seed["build_output_dir"],
            app_obj_suffix=shared_suffix,
            objroot_path=objroot_path,
            share_objects=True,
            verbose=True,
        )
        if json_entry is not None:
            size_results.append(json_entry)
        if failure is not None:
            failures.append(failure)
            for skipped in family_cases[1:]:
                failures.append(make_failure(skipped["case"], "warmup", "skipped after warmup failure"))
            continue

        parallel_tasks.extend(family_cases[1:])

    if not parallel_tasks:
        return size_results, failures

    with concurrent.futures.ProcessPoolExecutor(max_workers=min(parallel_jobs, len(parallel_tasks))) as executor:
        future_map = {}
        for item in parallel_tasks:
            future = executor.submit(
                process_case,
                0,
                total_work_cnt,
                item["case"],
                runtime_timeout,
                QEMU_MEASURE_CFLAGS,
                item["build_output_dir"],
                item["shared_suffix"],
                objroot_path if item["case"]["app"] in ("HelloBasic", "HelloVirtual", "HelloCustomWidgets", "HelloSizeAnalysis") else None,
                item["case"]["app"] in ("HelloBasic", "HelloVirtual", "HelloCustomWidgets", "HelloSizeAnalysis"),
                False,
            )
            future_map[future] = item["case"]

        for future in concurrent.futures.as_completed(future_map):
            case = future_map[future]
            completed += 1
            try:
                json_entry, failure = future.result()
            except Exception as exc:
                json_entry = None
                failure = make_failure(case, "unexpected", str(exc))
            if json_entry is not None:
                size_results.append(json_entry)
                print("[%d/%d] %s OK" % (completed, total_work_cnt, case["name"]))
            if failure is not None:
                failures.append(failure)
                print("[%d/%d] %s FAIL (%s)" % (completed, total_work_cnt, case["name"], failure["phase"]))

    return size_results, failures


def process_case(current_work_cnt, total_work_cnt, case, runtime_timeout, user_cflags="", build_output_dir=None,
                 app_obj_suffix=None, objroot_path=None, share_objects=False, verbose=True):
    if build_output_dir is None:
        build_output_dir = get_case_build_output_dir(case, "measure", user_cflags=user_cflags)
    if app_obj_suffix is None:
        app_obj_suffix = get_case_obj_suffix(case, "measure", user_cflags=user_cflags, share_objects=share_objects)
    resolved_build_output_dir = Path(build_output_dir)
    if not resolved_build_output_dir.is_absolute():
        resolved_build_output_dir = PROJECT_ROOT / resolved_build_output_dir

    if verbose:
        print("=================================================================================")
        print(
            "Total Work Cnt: %d, Current Cnt: %d, Process: %.2f%%, Case: %s"
            % (total_work_cnt, current_work_cnt, current_work_cnt * 100.0 / total_work_cnt, case["name"])
        )
        print("=================================================================================")

    # Optimization: Build only once with measure flags (saves 50% compile time)
    # The measure flags add minimal code that doesn't significantly affect size metrics
    measure_cmd, measure_result = build_case(
        case,
        "measure",
        QEMU_MEASURE_CFLAGS,
        build_output_dir=build_output_dir,
        app_obj_suffix=app_obj_suffix,
        objroot_path=objroot_path,
        share_objects=share_objects,
    )
    if verbose:
        print(format_cmd(measure_cmd))
    if measure_result.returncode != 0:
        message = (measure_result.stderr or measure_result.stdout)[-4000:]
        return None, make_failure(case, "build_measure", message, format_cmd(measure_cmd))

    elf_path = resolved_build_output_dir / "main.elf"
    map_path = resolved_build_output_dir / "main.map"

    if not elf_path.exists():
        return None, make_failure(case, "build_measure", "ELF not found after build", format_cmd(measure_cmd))

    if not map_path.exists():
        return None, make_failure(case, "build_measure", "Map not found after build", format_cmd(measure_cmd))

    # Extract repo-only static size info from the final linker map.
    elf_size_info = utils_process_map_file(str(map_path))

    sync_runtime_resource(case, resolved_build_output_dir)

    try:
        qemu_output = run_qemu(resolved_build_output_dir, runtime_timeout)
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
        "ram_bytes": elf_size_info.data_size + elf_size_info.bss_size,
        "pfb_bytes": elf_size_info.bss_pfb_size,
        "heap_idle_bytes": metrics["idle_current"],
        "heap_init_peak_bytes": metrics["idle_peak"],
        "heap_interaction_peak_bytes": metrics["interaction_total_peak"],
        "heap_peak_bytes": heap_peak_bytes,
        "stack_peak_bytes": metrics["stack_peak_bytes"],
        "interaction_action_count": metrics["interaction_action_count"],
    }
    shutil.rmtree(resolved_build_output_dir, ignore_errors=True)
    return json_entry, None


def build_readme(size_results, failures, case_set):
    lines = []
    lines.append("# QEMU Size Report")
    lines.append("")
    lines.append("- Build target: `PORT=qemu CPU_ARCH=%s`" % BUILD_CPU_ARCH)
    lines.append("- Runtime measurement: `qemu-system-arm -machine %s -cpu %s`" % (RUNTIME_MACHINE, RUNTIME_CPU))
    lines.append("- Scope: %s" % describe_scope(case_set))
    lines.append("- Static size scope: %s" % describe_static_size_scope())
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
    import argparse
    parser = argparse.ArgumentParser(
        description="EmbeddedGUI Binary Size and Memory Analysis"
    )
    parser.add_argument("--doc", action="store_true",
                        help="Generate documentation from analysis results")
    parser.add_argument(
        "--case-set",
        choices=("typical", "full"),
        default=DEFAULT_CASE_SET,
        help="Analysis scope: typical (fast representative set) or full (all configured cases). Default: %(default)s",
    )
    parser.add_argument(
        "--runtime-timeout",
        type=int,
        default=None,
        help="Per-case QEMU runtime timeout in seconds. Default: 60 for typical, 180 for full.",
    )
    parser.add_argument(
        "--jobs",
        type=int,
        default=0,
        help="Parallel size-analysis cases. 0=auto.",
    )
    args = parser.parse_args()

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    cases = discover_cases(args.case_set)
    total_work_cnt = len(cases)
    runtime_timeout = args.runtime_timeout
    if runtime_timeout is None:
        runtime_timeout = DEFAULT_RUNTIME_TIMEOUT_TYPICAL if args.case_set == "typical" else DEFAULT_RUNTIME_TIMEOUT_FULL

    try:
        size_results, failures = process_cases_parallel(cases, runtime_timeout, args.jobs)
    except ValueError as exc:
        print(str(exc))
        return 2

    README_PATH.write_text(build_readme(size_results, failures, args.case_set), encoding="utf-8")
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
            "case_set": args.case_set,
            "description": describe_scope(args.case_set),
            "cases": [case["name"] for case in cases],
        },
        "static_size_scope": {
            "source": "per-case build output main.map",
            "description": describe_static_size_scope(),
        },
        "apps": size_results,
        "failures": failures,
    }
    RESULT_JSON_PATH.write_text(json.dumps(json_data, indent=2, ensure_ascii=False), encoding="utf-8")
    print("JSON saved: %s" % RESULT_JSON_PATH)

    # Generate documentation from size analysis results (if --doc flag is set)
    if args.doc:
        print("\n=== Generating Documentation ===")
        try:
            if size_to_doc:
                original_argv = sys.argv[:]
                try:
                    sys.argv = [str(SCRIPT_DIR / "size_to_doc.py")]
                    size_to_doc.main()
                finally:
                    sys.argv = original_argv
                print("Documentation updated successfully")
            else:
                print("Warning: size_to_doc module not found")
        except Exception as e:
            print("Warning: Failed to generate documentation: %s" % e)
            import traceback
            traceback.print_exc()

    if failures:
        print("Completed with %d failures." % len(failures))
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
