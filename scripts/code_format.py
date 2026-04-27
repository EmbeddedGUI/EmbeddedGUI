import argparse
import os
import sys
import subprocess

def format_all_file(root):
    for root, dirs, files in os.walk(root):

        # root: current directory
        # dirs: subdirectories in current directory
        # files files in current directory

        # foreach file in current directory
        for f in files:
            if f.endswith('.c') or f.endswith('.h'):
                if '.venv' in root:
                    continue
                if 'tools' in root:
                    continue
                if '.claude' in root:
                    continue
                if '.eguiproject' in root:
                    continue
                if 'resource' in root:
                    continue
                if 'CMSIS' in root:
                    continue
                if 'Libraries' in root:
                    continue
                if 'sdl2' in root:
                    continue
                if 'STM32G0xx_HAL_Driver' in root:
                    continue
                if 'Core' in root:
                    continue
                if 'Drivers' in root:
                    continue
                if 'ref' in root:
                    continue
                if 'temp' in root:
                    continue
                if 'tmp' in root:
                    continue
                
                full_path = os.path.join(root, f)
                #print("root: %s, path: %s" % (root, full_path))
                print(full_path)
                
                command = ['clang-format', '-style=file', '-i', full_path]
                #print(command)
                proc = subprocess.run(command, stdout=subprocess.PIPE)

def parse_args(argv):
    parser = argparse.ArgumentParser(
        description='Format C/C++ source files with clang-format.',
    )
    parser.add_argument(
        'root',
        nargs='?',
        default='.',
        help='Root directory to scan. Defaults to the current directory.',
    )
    return parser.parse_args(argv)

def main(argv=None):
    args = parse_args(sys.argv[1:] if argv is None else argv)
    format_all_file(args.root)
    return 0

if __name__ == '__main__':
    sys.exit(main())

    #proc = subprocess.run(command, shell=True, stdout=subprocess.PIPE)
