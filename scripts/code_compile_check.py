import os
import sys
import argparse
import time
import shutil

mutil_work = True

# Speed up compilation: disable debug symbols, use -O0 (same as ui_designer)
COMPILE_FAST_FLAGS = ' COMPILE_DEBUG= COMPILE_OPT_LEVEL=-O0'

# Build system: 'make' or 'cmake'
build_system = 'make'

def get_example_list():
    path = 'example'
    app_list = []

    files = os.listdir(path)
    for file in files:
        file_path = os.path.join(path, file)

        if os.path.isdir(file_path):
            app_list.append(file)

    return sorted(app_list)

def get_example_basic_list():
    path = 'example/HelloBasic'
    app_list = []

    files = os.listdir(path)
    for file in files:
        file_path = os.path.join(path, file)

        if os.path.isdir(file_path):
            app_list.append(file)

    return sorted(app_list)

def compile_code(params):
    """Compile code using per-app OBJDIR (no make clean needed).

    PC Makefile uses APP_OBJ_SUFFIX so each APP gets its own obj directory.
    HelloBasic sub-apps share OBJDIR, so core library is compiled only once.
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


# port_sets = ['pc'
#              , 'stm32g0_empty'
#              ]
port_sets = ['pc'
             ]

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--full-check",
                        action="store_true",
                        default=False,
                        help="For normal build.")

    parser.add_argument("--actions",
                        action="store_true",
                        default=False,
                        help="For normal build.")

    parser.add_argument("--bits64",
                        action="store_true",
                        default=False,
                        help="For 64bit build.")

    parser.add_argument("--clean",
                        action="store_true",
                        default=False,
                        help="Clean output before building.")

    parser.add_argument("--cmake",
                        action="store_true",
                        default=False,
                        help="Use CMake build system instead of Make.")

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

    full_check = args.full_check
    if full_check:
        total_work_cnt = 0
        for app in app_sets:
            for port in port_sets:
                if app == "HelloBasic":
                    for app_basic in app_basic_sets:
                        total_work_cnt += 1
                else:
                    total_work_cnt += 1

        current_work_cnt = 0
        for app in app_sets:
            for port in port_sets:
                if app == "HelloBasic":
                    for app_basic in app_basic_sets:
                            current_work_cnt += 1
                            process_app(current_work_cnt, total_work_cnt, app, port, app_basic, params)
                else:
                    current_work_cnt += 1
                    process_app(current_work_cnt, total_work_cnt, app, port, None, params)

    # Run unit tests
    res = run_unit_tests(params)
    if res != 0:
        sys.exit(res)

    elapsed = time.time() - start_time
    print("=================================================================================")
    print("All checks passed! Time: %.1fs" % elapsed)
    print("=================================================================================")

    sys.exit(0)
