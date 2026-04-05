"""Compile-check helper for EmbeddedGUI examples and widget demos."""

import os
import sys
import argparse
import time
import shutil
import subprocess

mutil_work = True

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

def compile_code(params):
    """Compile code using per-app OBJDIR (no make clean needed).

    PC Makefile uses APP_OBJ_SUFFIX so each APP gets its own obj directory.
    HelloBasic/HelloVirtual/HelloSizeAnalysis sub-apps use dedicated OBJDIRs per sub-app.
    """
    if build_system == 'cmake':
        return compile_code_cmake(params)

    if mutil_work:
        cmd = 'make -j' + params + COMPILE_FAST_FLAGS
    else:
        cmd = 'make' + params + COMPILE_FAST_FLAGS
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
    if mutil_work:
        build_cmd = 'cmake --build %s -j' % build_dir
    else:
        build_cmd = 'cmake --build %s' % build_dir
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
        ),
    )
    parser.add_argument("--full-check",
                        action="store_true",
                        default=False,
                        help="Compile all tracked standard examples plus HelloBasic/HelloVirtual/HelloSizeAnalysis sub-apps.")

    parser.add_argument("--actions",
                        action="store_true",
                        default=False,
                        help="Use GitHub Actions friendly settings (single-threaded app loop).")

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

    parser.add_argument("--skip-icon-font-check",
                        action="store_true",
                        default=False,
                        help="Skip example icon font explicitness check in full-check mode.")

    parser.add_argument("--category",
                        type=str,
                        default=None,
                        help="Only check specific category (e.g. input, display).")

    return parser.parse_args()

def process_app(current_work_cnt, total_work_cnt, app, port, app_basic, params):
    print("=================================================================================")
    print("Total Work Cnt: %d, Current Cnt: %d, Process: %.2f%%"
        % (total_work_cnt, current_work_cnt, current_work_cnt * 100.0 / total_work_cnt))
    print("=================================================================================")

    if app_basic != None:
        params_full = params + (' APP=%s PORT=%s APP_SUB=%s') % (app, port, app_basic)
    else:
        params_full = params + (' APP=%s PORT=%s') % (app, port)

    res = compile_code(params_full)
    if(res != 0):
        sys.exit(res)


if __name__ == '__main__':
    args = parse_args()

    actions = args.actions
    if actions:
        port_sets = ['pc']
        mutil_work = False

    # Select build system
    if args.cmake:
        build_system = 'cmake'

    #params = ' V=1'
    params = ''

    if args.bits64:
        params += ' BITS=64'

    app_sets = get_example_list()
    app_basic_sets = get_example_basic_list()
    app_virtual_sets = get_example_virtual_list()
    app_size_analysis_sets = get_example_size_analysis_list()

    # Clean once at start if requested (or for backward compat on first run)
    if args.clean:
        if build_system == 'cmake':
            if os.path.exists('build_cmake'):
                shutil.rmtree('build_cmake')
        else:
            os.system('make clean')

    start_time = time.time()

    res = compile_code(params)
    if(res != 0):
        sys.exit(res)

    # Custom widgets check mode
    if args.custom_widgets:
        res = run_touch_release_semantics_check(scope='custom', category=args.category)
        if res != 0:
            sys.exit(res)

        custom_list = get_custom_widgets_list(args.category)
        total_work_cnt = len(custom_list)
        current_work_cnt = 0
        for widget_sub in custom_list:
            current_work_cnt += 1
            process_app(current_work_cnt, total_work_cnt, "HelloCustomWidgets", "pc", widget_sub, params)

        elapsed = time.time() - start_time
        print("=================================================================================")
        print("Custom widgets check passed! Time: %.1fs" % elapsed)
        print("=================================================================================")
        sys.exit(0)

    full_check = args.full_check
    if full_check:
        res = run_touch_release_semantics_check(scope='all')
        if res != 0:
            sys.exit(res)

        total_work_cnt = 0
        for app in app_sets:
            for port in port_sets:
                if app == "HelloCustomWidgets":
                    # Custom widgets checked separately via --custom-widgets
                    continue
                if app == "HelloBasic":
                    for app_basic in app_basic_sets:
                        total_work_cnt += 1
                elif app == "HelloVirtual":
                    for app_virtual in app_virtual_sets:
                        total_work_cnt += 1
                elif app == "HelloSizeAnalysis":
                    for app_probe in app_size_analysis_sets:
                        total_work_cnt += 1
                else:
                    total_work_cnt += 1

        current_work_cnt = 0
        for app in app_sets:
            for port in port_sets:
                if app == "HelloCustomWidgets":
                    # Custom widgets checked separately via --custom-widgets
                    continue
                if app == "HelloBasic":
                    for app_basic in app_basic_sets:
                        current_work_cnt += 1
                        process_app(current_work_cnt, total_work_cnt, app, port, app_basic, params)
                elif app == "HelloVirtual":
                    for app_virtual in app_virtual_sets:
                        current_work_cnt += 1
                        process_app(current_work_cnt, total_work_cnt, app, port, app_virtual, params)
                elif app == "HelloSizeAnalysis":
                    for app_probe in app_size_analysis_sets:
                        current_work_cnt += 1
                        process_app(current_work_cnt, total_work_cnt, app, port, app_probe, params)
                else:
                    current_work_cnt += 1
                    process_app(current_work_cnt, total_work_cnt, app, port, None, params)

        if not args.skip_icon_font_check:
            res = run_example_icon_font_check()
            if res != 0:
                sys.exit(res)

    # Run unit tests
    res = run_unit_tests(params)
    if res != 0:
        sys.exit(res)

    elapsed = time.time() - start_time
    print("=================================================================================")
    print("All checks passed! Time: %.1fs" % elapsed)
    print("=================================================================================")

    sys.exit(0)
