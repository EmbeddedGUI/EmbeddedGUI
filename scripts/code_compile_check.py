"""Compile-check helper for EmbeddedGUI examples and widget demos."""

import concurrent.futures
import hashlib
import json
import os
import sys
import argparse
import time
import shutil
import subprocess
from pathlib import Path

make_jobs = None

# Speed up compilation: disable debug symbols, use -O0 (same as ui_designer)
COMPILE_FAST_FLAGS = ' COMPILE_DEBUG= COMPILE_OPT_LEVEL=-O0'

# Build system: 'make' or 'cmake'
build_system = 'make'
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = Path(SCRIPT_DIR).parent
APP_SUB_ROOTS = {
    "HelloBasic": "example/HelloBasic",
    "HelloVirtual": "example/HelloVirtual",
    "HelloSizeAnalysis": "example/HelloSizeAnalysis",
}
SUB_APP_FAMILY_APPS = tuple(APP_SUB_ROOTS.keys())
FULL_CHECK_SKIP_APPS = {"HelloCustomWidgets"}
COMPILE_OUTPUT_ROOT = Path("output") / "cc"

def get_example_list():
    path = 'example'
    app_list = []

    files = os.listdir(path)
    for file in files:
        file_path = os.path.join(path, file)

        if os.path.isdir(file_path) and os.path.exists(os.path.join(file_path, 'build.mk')):
            app_list.append(file)

    return sorted(app_list)

def get_example_sub_list(app):
    path = APP_SUB_ROOTS.get(app)
    app_list = []

    if not path or not os.path.isdir(path):
        return app_list

    files = os.listdir(path)
    for file in files:
        file_path = os.path.join(path, file)

        if os.path.isdir(file_path) and os.path.exists(os.path.join(file_path, 'app_egui_config.h')):
            app_list.append(file)

    return sorted(app_list)

def get_example_basic_list():
    return get_example_sub_list("HelloBasic")


def get_example_virtual_list():
    return get_example_sub_list("HelloVirtual")


def get_example_size_analysis_list():
    return get_example_sub_list("HelloSizeAnalysis")

def get_custom_widgets_list(category=None):
    """Discover HelloCustomWidgets sub-apps (category/widget_name pairs)."""
    base = 'example/HelloCustomWidgets'
    if not os.path.isdir(base):
        return []

    result = []
    categories = os.listdir(base)
    for cat in sorted(categories):
        cat_path = os.path.join(base, cat)
        if not os.path.isdir(cat_path):
            continue
        if category and cat != category:
            continue
        for widget in sorted(os.listdir(cat_path)):
            widget_path = os.path.join(cat_path, widget)
            if os.path.isdir(widget_path) and os.path.exists(os.path.join(widget_path, 'test.c')):
                result.append(f"{cat}/{widget}")
    return result


def normalize_user_cflags(user_cflags):
    return " ".join((user_cflags or "").split())


def get_config_paths(app, app_sub=None):
    config_paths = []
    if app_sub:
        config_paths.append(ROOT_DIR / "example" / app / app_sub / "app_egui_config.h")
    config_paths.append(ROOT_DIR / "example" / app / "app_egui_config.h")
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


def get_compile_output_signature(app, port, app_sub=None, bits64=False, user_cflags=""):
    payload = {
        "app": app,
        "port": port,
        "app_sub": app_sub or "",
        "bits64": bool(bits64),
        "user_cflags": normalize_user_cflags(user_cflags),
        "build_system": build_system,
    }
    return hashlib.sha1(json.dumps(payload, sort_keys=True, separators=(",", ":")).encode("utf-8")).hexdigest()[:10]


def get_compile_output_dir(app, port, app_sub=None, bits64=False, user_cflags=""):
    return COMPILE_OUTPUT_ROOT / get_compile_output_signature(app, port, app_sub=app_sub, bits64=bits64, user_cflags=user_cflags)


def get_compile_obj_suffix(app, port, app_sub=None, bits64=False, user_cflags=""):
    scope_hash = hashlib.sha1(
        json.dumps(
            {
                "port": port,
                "bits64": bool(bits64),
                "user_cflags": normalize_user_cflags(user_cflags),
            },
            sort_keys=True,
            separators=(",", ":"),
        ).encode("utf-8")
    ).hexdigest()[:8]
    if app in SUB_APP_FAMILY_APPS or app == "HelloCustomWidgets":
        return "cc_%s_cfg_%s_%s" % (app.lower(), get_config_hash(app, app_sub), scope_hash)
    return "cc_%s_%s" % (app.lower(), get_compile_output_signature(app, port, app_sub=app_sub, bits64=bits64, user_cflags=user_cflags))


def get_make_parallel_arg():
    if make_jobs is None:
        return "-j"
    if make_jobs <= 1:
        return ""
    return "-j%d" % make_jobs


def resolve_make_jobs(actions, requested_jobs):
    if requested_jobs < 0:
        raise ValueError("--jobs must be >= 0")

    if requested_jobs > 0:
        return requested_jobs

    if actions:
        cpu_count = os.cpu_count() or 1
        return min(2, max(1, cpu_count))

    return None


def get_auto_case_parallel_jobs(total_cases):
    if total_cases <= 1:
        return 1
    cpu_count = os.cpu_count() or 1
    return max(1, min(total_cases, 8, cpu_count // 4 or 1))


def resolve_case_parallel_jobs(requested_jobs, total_cases):
    if requested_jobs is not None and requested_jobs < 0:
        raise ValueError("--case-jobs must be >= 0")
    if total_cases <= 1:
        return 1
    if requested_jobs and requested_jobs > 0:
        return max(1, min(total_cases, requested_jobs))
    return get_auto_case_parallel_jobs(total_cases)


def build_sub_app_sets():
    return {
        "HelloBasic": get_example_basic_list(),
        "HelloVirtual": get_example_virtual_list(),
        "HelloSizeAnalysis": get_example_size_analysis_list(),
    }


def expand_app_cases(app, port, sub_app_sets):
    if app in sub_app_sets:
        return [(app, port, app_sub) for app_sub in sub_app_sets[app]]
    return [(app, port, None)]


def build_compile_cases_for_apps(apps, ports, sub_app_sets):
    cases = []
    for app in apps:
        for port in ports:
            cases.extend(expand_app_cases(app, port, sub_app_sets))
    return cases


def build_standard_app_list(app_sets):
    return [app for app in app_sets if app not in SUB_APP_FAMILY_APPS and app not in FULL_CHECK_SKIP_APPS]


def build_compile_cases(scope, app_sets, ports, sub_app_sets):
    if scope == "standard":
        selected_apps = build_standard_app_list(app_sets)
    elif scope == "basic":
        selected_apps = ["HelloBasic"]
    elif scope == "virtual":
        selected_apps = ["HelloVirtual"]
    elif scope == "size-analysis":
        selected_apps = ["HelloSizeAnalysis"]
    elif scope == "full":
        selected_apps = [app for app in app_sets if app not in FULL_CHECK_SKIP_APPS]
    else:
        raise ValueError("unknown scope: %s" % scope)

    return build_compile_cases_for_apps(selected_apps, ports, sub_app_sets)


def apply_case_shard(cases, shard_count, shard_index):
    if shard_count <= 1:
        return cases

    if shard_index < 1 or shard_index > shard_count:
        raise ValueError("--shard-index must be in [1, --shard-count]")

    shard_cases = [case for index, case in enumerate(cases) if index % shard_count == (shard_index - 1)]
    return shard_cases

def compile_code(params, output_dir=None, objroot_dir=None, app_obj_suffix=None):
    """Compile code using per-app OBJDIR (no make clean needed).

    PC Makefile uses APP_OBJ_SUFFIX so each APP gets its own obj directory.
    HelloBasic/HelloVirtual/HelloSizeAnalysis sub-apps use dedicated OBJDIRs per sub-app.
    """
    if build_system == 'cmake':
        return compile_code_cmake(params)

    cmd = 'make'
    parallel_arg = get_make_parallel_arg()
    if parallel_arg:
        cmd += ' ' + parallel_arg
    cmd += params + COMPILE_FAST_FLAGS
    if output_dir is not None:
        cmd += ' OUTPUT_PATH=%s' % Path(output_dir).as_posix()
    if objroot_dir is not None:
        cmd += ' OBJROOT_PATH=%s' % Path(objroot_dir).as_posix()
    if app_obj_suffix:
        cmd += ' APP_OBJ_SUFFIX=%s' % app_obj_suffix
    print(cmd)
    res = os.system(cmd)
    if res != 0:
        return res

    return 0


def compile_code_cmake(params):
    """Compile code using CMake build system."""
    # Parse params to extract APP, PORT, APP_SUB, BITS
    app = 'HelloSimple'
    port = 'pc'
    app_sub = ''
    bits = ''

    parts = params.split()
    for part in parts:
        if part.startswith('APP='):
            app = part[4:]
        elif part.startswith('PORT='):
            port = part[5:]
        elif part.startswith('APP_SUB='):
            app_sub = part[8:]
        elif part.startswith('BITS='):
            bits = part[5:]

    # Build directory name includes app (and sub) to avoid conflicts
    if app_sub:
        build_dir = 'build_cmake/%s_%s' % (app, app_sub)
    elif port == 'pc_test':
        build_dir = 'build_cmake/%s_test' % app
    else:
        build_dir = 'build_cmake/%s' % app

    # CMake configure
    cmake_args = [
        'cmake',
        '-B', build_dir,
        '-DAPP=%s' % app,
        '-DPORT=%s' % port,
    ]
    if app_sub:
        cmake_args.append('-DAPP_SUB=%s' % app_sub)

    # Use MinGW Makefiles on Windows to match gcc toolchain
    if os.name == 'nt':
        cmake_args.extend(['-G', '"MinGW Makefiles"'])

    cmd = ' '.join(cmake_args)
    print(cmd)
    res = os.system(cmd)
    if res != 0:
        return res

    # CMake build
    build_cmd = 'cmake --build %s' % build_dir
    parallel_arg = get_make_parallel_arg()
    if parallel_arg:
        build_cmd += ' ' + parallel_arg
    print(build_cmd)
    res = os.system(build_cmd)
    if res != 0:
        return res

    return 0

def run_unit_tests(params):
    print("=================================================================================")
    print("Running Unit Tests")
    print("=================================================================================")

    if build_system == 'cmake':
        # CMake: build with pc_test port, use separate build dir
        test_build_dir = 'build_cmake/HelloUnitTest_test'
        if os.path.exists(test_build_dir):
            shutil.rmtree(test_build_dir)
        test_params = params + ' APP=HelloUnitTest PORT=pc_test'
        res = compile_code(test_params)
        if res != 0:
            print("Unit test build failed!")
            return res

        # Run the test executable
        if os.name == 'nt':
            cmd = os.path.normpath(os.path.join(test_build_dir, 'output', 'HelloUnitTest.exe'))
        else:
            cmd = os.path.join(test_build_dir, 'output', 'HelloUnitTest')
        print(cmd)
        res = os.system(cmd)
    else:
        # Force clean to ensure test build is fresh and not affected by previous builds (e.g., if test-specific files were added/removed, or to catch missing dependencies).
        os.system('make clean')
        # Build with headless test port
        test_params = params + ' APP=HelloUnitTest PORT=pc_test'
        res = compile_code(test_params)
        if res != 0:
            print("Unit test build failed!")
            return res

        # Run the test executable
        if os.name == 'nt':
            cmd = 'output\\main.exe'
        else:
            cmd = './output/main'
        print(cmd)
        res = os.system(cmd)

    if res != 0:
        print("Unit tests FAILED!")
        return res

    print("Unit tests PASSED!")
    return 0


def run_example_icon_font_check():
    print("=================================================================================")
    print("Checking Example Icon Fonts")
    print("=================================================================================")

    cmd = [sys.executable, os.path.join(SCRIPT_DIR, 'checks', 'check_example_icon_font.py')]
    print(' '.join('"%s"' % part if ' ' in part else part for part in cmd))
    res = subprocess.call(cmd)
    if res != 0:
        print("Example icon font check FAILED!")
        return res

    print("Example icon font check PASSED!")
    return 0


def run_touch_release_semantics_check(scope='all', category=None):
    print("=================================================================================")
    print("Checking Widget Touch Release Semantics")
    print("=================================================================================")

    cmd = [sys.executable, os.path.join(SCRIPT_DIR, 'checks', 'check_touch_release_semantics.py')]
    if scope:
        cmd.extend(['--scope', scope])
    if category:
        cmd.extend(['--category', category])
    print(' '.join('"%s"' % part if ' ' in part else part for part in cmd))
    res = subprocess.call(cmd)
    if res != 0:
        print("Widget touch release semantics check FAILED!")
        return res

    print("Widget touch release semantics check PASSED!")
    return 0


# port_sets = ['pc'
#              , 'stm32g0'
#              ]
port_sets = ['pc'
             ]

def parse_args():
    parser = argparse.ArgumentParser(
        description="Compile EmbeddedGUI examples, optional custom widget demos, and unit tests.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Examples:\n"
            "  python scripts/code_compile_check.py --full-check\n"
            "  python scripts/code_compile_check.py --full-check --cmake\n"
            "  python scripts/code_compile_check.py --custom-widgets --category input\n"
            "  python scripts/code_compile_check.py --scope basic --shard-count 3 --shard-index 1 --actions --bits64\n"
            "  python scripts/code_compile_check.py --unit-tests-only --bits64\n"
        ),
    )
    parser.add_argument("--full-check",
                        action="store_true",
                        default=False,
                        help="Compile all tracked standard examples plus HelloBasic/HelloVirtual/HelloSizeAnalysis sub-apps.")

    parser.add_argument("--scope",
                        choices=["standard", "basic", "virtual", "size-analysis"],
                        default=None,
                        help="Compile only one case family, useful for CI sharding.")

    parser.add_argument("--actions",
                        action="store_true",
                        default=False,
                        help="Use GitHub Actions friendly settings.")

    parser.add_argument("--bits64",
                        action="store_true",
                        default=False,
                        help="Build 64-bit binaries.")

    parser.add_argument("--clean",
                        action="store_true",
                        default=False,
                        help="Clean existing build output before compiling.")

    parser.add_argument("--cmake",
                        action="store_true",
                        default=False,
                        help="Use CMake build system instead of Make.")

    parser.add_argument("--custom-widgets",
                        action="store_true",
                        default=False,
                        help="Compile HelloCustomWidgets demos instead of standard example apps.")

    parser.add_argument("--unit-tests-only",
                        action="store_true",
                        default=False,
                        help="Only build and run HelloUnitTest on pc_test.")

    parser.add_argument("--skip-icon-font-check",
                        action="store_true",
                        default=False,
                        help="Skip example icon font explicitness check in full-check mode.")

    parser.add_argument("--category",
                        type=str,
                        default=None,
                        help="Only check specific category (e.g. input, display).")

    parser.add_argument("--jobs",
                        type=int,
                        default=0,
                        help="Per-build make -j jobs. 0=auto (Actions defaults to 2, local keeps Make default unless case parallelism needs throttling).")

    parser.add_argument("--case-jobs",
                        type=int,
                        default=0,
                        help="Parallel compile cases. 0=auto.")

    parser.add_argument("--shard-count",
                        type=int,
                        default=1,
                        help="Split scoped compile cases into N shards.")

    parser.add_argument("--shard-index",
                        type=int,
                        default=1,
                        help="1-based shard index used with --shard-count.")

    return parser.parse_args()

def process_app(current_work_cnt, total_work_cnt, app, port, app_sub, params):
    print("=================================================================================")
    print("Total Work Cnt: %d, Current Cnt: %d, Process: %.2f%%"
        % (total_work_cnt, current_work_cnt, current_work_cnt * 100.0 / total_work_cnt))
    print("=================================================================================")

    if app_sub is not None:
        params_full = params + (' APP=%s PORT=%s APP_SUB=%s') % (app, port, app_sub)
    else:
        params_full = params + (' APP=%s PORT=%s') % (app, port)

    res = compile_code(params_full)
    if(res != 0):
        sys.exit(res)


def build_case_params(params, app, port, app_sub):
    if app_sub is not None:
        return params + (' APP=%s PORT=%s APP_SUB=%s') % (app, port, app_sub)
    return params + (' APP=%s PORT=%s') % (app, port)


def compile_case_worker(app, port, app_sub, params, bits64, user_cflags, output_dir, objroot_dir, app_obj_suffix):
    params_full = build_case_params(params, app, port, app_sub)
    res = compile_code(
        params_full,
        output_dir=output_dir,
        objroot_dir=objroot_dir,
        app_obj_suffix=app_obj_suffix,
    )
    return {
        "name": format_app_name(app, app_sub),
        "returncode": res,
        "output_dir": str(output_dir) if output_dir is not None else "",
    }


def format_app_name(app, app_sub=None):
    if not app_sub:
        return app
    return "%s_%s" % (app, app_sub.replace("\\", "_").replace("/", "_"))


def run_compile_cases_parallel(cases, params, bits64=False, user_cflags="", case_jobs=0):
    global make_jobs

    total_work_cnt = len(cases)
    if total_work_cnt == 0:
        print("No compile cases selected.")
        return

    if build_system == 'cmake':
        run_compile_cases(cases, params)
        return

    parallel_jobs = resolve_case_parallel_jobs(case_jobs, total_work_cnt)
    if parallel_jobs <= 1:
        run_compile_cases(cases, params)
        return

    cpu_count = os.cpu_count() or 1
    if make_jobs is None:
        effective_make_jobs = max(1, cpu_count // parallel_jobs)
    else:
        effective_make_jobs = make_jobs

    previous_make_jobs = make_jobs
    make_jobs = effective_make_jobs

    try:
        objroot_dir = Path("output")
        grouped_cases = {}
        direct_cases = []
        for app, port, app_sub in cases:
            output_dir = get_compile_output_dir(app, port, app_sub=app_sub, bits64=bits64, user_cflags=user_cflags)
            obj_suffix = get_compile_obj_suffix(app, port, app_sub=app_sub, bits64=bits64, user_cflags=user_cflags)
            case_info = {
                "app": app,
                "port": port,
                "app_sub": app_sub,
                "output_dir": output_dir,
                "obj_suffix": obj_suffix,
            }
            if app in SUB_APP_FAMILY_APPS or app == "HelloCustomWidgets":
                grouped_cases.setdefault(obj_suffix, []).append(case_info)
            else:
                direct_cases.append(case_info)

        print("Running compile cases with jobs=%d, make -j%d" % (parallel_jobs, effective_make_jobs))
        results = []
        completed = 0

        parallel_tasks = []
        for obj_suffix in sorted(grouped_cases):
            family_cases = grouped_cases[obj_suffix]
            seed_case = family_cases[0]
            completed += 1
            seed_name = format_app_name(seed_case["app"], seed_case["app_sub"])
            print("=================================================================================")
            print("Total Work Cnt: %d, Current Cnt: %d, Process: %.2f%%" % (total_work_cnt, completed, completed * 100.0 / total_work_cnt))
            print("=================================================================================")
            print("%s (warmup)" % seed_name)
            result = compile_case_worker(
                seed_case["app"],
                seed_case["port"],
                seed_case["app_sub"],
                params,
                bits64,
                user_cflags,
                seed_case["output_dir"],
                objroot_dir,
                seed_case["obj_suffix"],
            )
            if result["returncode"] != 0:
                sys.exit(result["returncode"])
            shutil.rmtree(seed_case["output_dir"], ignore_errors=True)
            for case_info in family_cases[1:]:
                parallel_tasks.append(case_info)

        parallel_tasks.extend(direct_cases)

        if not parallel_tasks:
            return

        with concurrent.futures.ProcessPoolExecutor(max_workers=min(parallel_jobs, len(parallel_tasks))) as executor:
            future_map = {}
            for case_info in parallel_tasks:
                future = executor.submit(
                    compile_case_worker,
                    case_info["app"],
                    case_info["port"],
                    case_info["app_sub"],
                    params,
                    bits64,
                    user_cflags,
                    case_info["output_dir"],
                    objroot_dir,
                    case_info["obj_suffix"],
                )
                future_map[future] = case_info

            for future in concurrent.futures.as_completed(future_map):
                case_info = future_map[future]
                result = future.result()
                completed += 1
                name = result["name"]
                if result["returncode"] != 0:
                    print("[%d/%d] %s FAILED" % (completed, total_work_cnt, name))
                    sys.exit(result["returncode"])
                print("[%d/%d] %s OK" % (completed, total_work_cnt, name))
                shutil.rmtree(case_info["output_dir"], ignore_errors=True)
                results.append(result)
    finally:
        make_jobs = previous_make_jobs


def run_compile_cases(cases, params):
    total_work_cnt = len(cases)
    if total_work_cnt == 0:
        print("No compile cases selected.")
        return

    for current_work_cnt, (app, port, app_sub) in enumerate(cases, start=1):
        process_app(current_work_cnt, total_work_cnt, app, port, app_sub, params)


if __name__ == '__main__':
    args = parse_args()

    if args.full_check and args.scope is not None:
        print("Error: --full-check cannot be combined with --scope")
        sys.exit(1)

    if args.custom_widgets and args.scope is not None:
        print("Error: --custom-widgets cannot be combined with --scope")
        sys.exit(1)

    if args.unit_tests_only and (args.full_check or args.custom_widgets or args.scope is not None):
        print("Error: --unit-tests-only cannot be combined with compile scopes")
        sys.exit(1)

    if args.shard_count < 1:
        print("Error: --shard-count must be >= 1")
        sys.exit(1)

    if args.scope is None and args.shard_count != 1:
        print("Error: --shard-count requires --scope")
        sys.exit(1)

    if args.scope is None and args.shard_index != 1:
        print("Error: --shard-index requires --scope")
        sys.exit(1)

    if args.scope is not None and args.shard_count == 1 and args.shard_index != 1:
        print("Error: --shard-index must be 1 when --shard-count is 1")
        sys.exit(1)

    actions = args.actions
    if actions:
        port_sets = ['pc']

    try:
        make_jobs = resolve_make_jobs(actions, args.jobs)
    except ValueError as exc:
        print("Error: %s" % exc)
        sys.exit(1)

    try:
        resolve_case_parallel_jobs(args.case_jobs, 2)
    except ValueError as exc:
        print("Error: %s" % exc)
        sys.exit(1)

    # Select build system
    if args.cmake:
        build_system = 'cmake'

    #params = ' V=1'
    params = ''

    if args.bits64:
        params += ' BITS=64'

    app_sets = get_example_list()
    sub_app_sets = build_sub_app_sets()

    # Clean once at start if requested (or for backward compat on first run)
    if args.clean:
        if build_system == 'cmake':
            if os.path.exists('build_cmake'):
                shutil.rmtree('build_cmake')
        else:
            os.system('make clean')

    start_time = time.time()

    # Custom widgets check mode
    if args.custom_widgets:
        res = run_touch_release_semantics_check(scope='custom', category=args.category)
        if res != 0:
            sys.exit(res)

        custom_list = get_custom_widgets_list(args.category)
        custom_cases = [("HelloCustomWidgets", "pc", widget_sub) for widget_sub in custom_list]
        run_compile_cases_parallel(custom_cases, params, bits64=args.bits64, case_jobs=args.case_jobs)

        elapsed = time.time() - start_time
        print("=================================================================================")
        print("Custom widgets check passed! Time: %.1fs" % elapsed)
        print("=================================================================================")
        sys.exit(0)

    if args.unit_tests_only:
        res = run_unit_tests(params)
        if res != 0:
            sys.exit(res)

        elapsed = time.time() - start_time
        print("=================================================================================")
        print("Unit tests passed! Time: %.1fs" % elapsed)
        print("=================================================================================")
        sys.exit(0)

    if args.full_check:
        res = run_touch_release_semantics_check(scope='all')
        if res != 0:
            sys.exit(res)

        full_cases = build_compile_cases("full", app_sets, port_sets, sub_app_sets)
        run_compile_cases_parallel(full_cases, params, bits64=args.bits64, case_jobs=args.case_jobs)

        if not args.skip_icon_font_check:
            res = run_example_icon_font_check()
            if res != 0:
                sys.exit(res)
        res = run_unit_tests(params)
        if res != 0:
            sys.exit(res)

        elapsed = time.time() - start_time
        print("=================================================================================")
        print("All checks passed! Time: %.1fs" % elapsed)
        print("=================================================================================")
        sys.exit(0)

    if args.scope is not None:
        try:
            scope_cases = build_compile_cases(args.scope, app_sets, port_sets, sub_app_sets)
            scope_cases = apply_case_shard(scope_cases, args.shard_count, args.shard_index)
        except ValueError as exc:
            print("Error: %s" % exc)
            sys.exit(1)

        print("Selected scope=%s shard=%d/%d cases=%d" % (
            args.scope,
            args.shard_index,
            args.shard_count,
            len(scope_cases),
        ))
        run_compile_cases_parallel(scope_cases, params, bits64=args.bits64, case_jobs=args.case_jobs)

        elapsed = time.time() - start_time
        print("=================================================================================")
        print("Scope compile passed! Time: %.1fs" % elapsed)
        print("=================================================================================")
        sys.exit(0)

    res = compile_code(params)
    if(res != 0):
        sys.exit(res)

    res = run_unit_tests(params)
    if res != 0:
        sys.exit(res)

    elapsed = time.time() - start_time
    print("=================================================================================")
    print("All checks passed! Time: %.1fs" % elapsed)
    print("=================================================================================")

    sys.exit(0)
