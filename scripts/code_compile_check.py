"""Compile-check helper for EmbeddedGUI examples and widget demos."""

import os
import sys
import argparse
import time
import shutil
import subprocess

make_jobs = None

# Speed up compilation: disable debug symbols, use -O0 (same as ui_designer)
COMPILE_FAST_FLAGS = ' COMPILE_DEBUG= COMPILE_OPT_LEVEL=-O0'

# Build system: 'make' or 'cmake'
build_system = 'make'
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
APP_SUB_ROOTS = {
    "HelloBasic": "example/HelloBasic",
    "HelloVirtual": "example/HelloVirtual",
    "HelloSizeAnalysis": "example/HelloSizeAnalysis",
}
SUB_APP_FAMILY_APPS = tuple(APP_SUB_ROOTS.keys())
FULL_CHECK_SKIP_APPS = {"HelloCustomWidgets"}

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

def compile_code(params):
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
                        help="Parallel build jobs. 0=auto (Actions defaults to 2, local preserves make -j).")

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
        run_compile_cases(custom_cases, params)

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
        run_compile_cases(full_cases, params)

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
        run_compile_cases(scope_cases, params)

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
