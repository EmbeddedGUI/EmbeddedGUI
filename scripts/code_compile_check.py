import os
import sys
import argparse

def get_example_list():
    path = 'example'
    app_list = []

    files = os.listdir(path)
    for file in files:
        file_path = os.path.join(path, file)

        if os.path.isdir(file_path):
            app_list.append(file)
    
    return app_list

def get_example_basic_list():
    path = 'example/HelloBasic'
    app_list = []

    files = os.listdir(path)
    for file in files:
        file_path = os.path.join(path, file)

        if os.path.isdir(file_path):
            app_list.append(file)
    
    return app_list

def compile_code(params):
    # compile code
    cmd = 'make clean'
    res = os.system(cmd)
    if res != 0:
        return res
    cmd = 'make all -j' + params
    print(cmd)
    res = os.system(cmd)
    if res != 0:
        return res

    return 0


port_sets = ['pc', 'stm32g0_empty']

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--full-check",
                        action="store_true",
                        default=False,
                        help="For normal build.")
                        
    return parser.parse_args()

def process_app(current_work_cnt, total_work_cnt, app, port, app_basic, params):
    current_work_cnt += 1
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

    #params = ' V=1'
    params = ''

    app_sets = get_example_list()
    app_basic_sets = get_example_basic_list()

    res = compile_code(params)
    if(res != 0):
        sys.exit(res)

    full_check = args.full_check
    full_check = True
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
                            process_app(current_work_cnt, total_work_cnt, app, port, app_basic, params)
                else:
                    process_app(current_work_cnt, total_work_cnt, app, port, None, params)
    
    sys.exit(0)