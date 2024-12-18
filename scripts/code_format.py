import os
import subprocess
import re
import sys

def format_all_file(root):
    for root, dirs, files in os.walk(root):

        # root: current directory
        # dirs: subdirectories in current directory
        # files files in current directory

        # foreach file in current directory
        for f in files:
            if f.endswith('.c') or f.endswith('.h'):
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
                
                full_path = os.path.join(root, f)
                #print("root: %s, path: %s" % (root, full_path))
                print(full_path)
                
                command = 'clang-format -style=file -i %s' % full_path
                #print(command)
                proc = subprocess.run(command, shell=True, stdout=subprocess.PIPE)

if __name__ == '__main__':
    format_all_file('.')

    #proc = subprocess.run(command, shell=True, stdout=subprocess.PIPE)
