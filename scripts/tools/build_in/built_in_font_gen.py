#!/usr/bin/env python3

import subprocess
import sys
from pathlib import Path


BUILD_IN_DIR = Path(__file__).resolve().parent
TOOLS_DIR = BUILD_IN_DIR.parent
ROOT_DIR = BUILD_IN_DIR.parents[2]
RESOURCE_DIR = ROOT_DIR / "src" / "resource"
TTF2C_PATH = TOOLS_DIR / "ttf2c.py"
ICON_GEN_PATH = BUILD_IN_DIR / "built_in_icon_font_gen.py"


def run_tool(args):
    subprocess.run(args, cwd=BUILD_IN_DIR, check=True)


def generate_montserrat():
    for size in range(8, 50, 2):
        print(f"Generating Montserrat {size}px")
        run_tool(
            [
                sys.executable,
                str(TTF2C_PATH),
                "-i",
                "Montserrat-Medium.ttf",
                "-n",
                "montserrat",
                "-t",
                "supported_text.txt",
                "-p",
                str(size),
                "-s",
                "4",
                "-o",
                str(RESOURCE_DIR),
            ]
        )


def main():
    generate_montserrat()
    run_tool([sys.executable, str(ICON_GEN_PATH)])


if __name__ == "__main__":
    main()



