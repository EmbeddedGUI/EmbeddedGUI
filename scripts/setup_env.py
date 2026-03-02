#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Environment setup helper for EmbeddedGUI.
Handles:
  - w64devkit (GCC + Make) download and extraction
  - Python virtual environment creation and dependency installation

Usage:
    python scripts/setup_env.py --mode 1               # Basic Python deps only
    python scripts/setup_env.py --mode 2               # Basic + UI Designer deps
    python scripts/setup_env.py --install-toolchain     # Download w64devkit
"""

import os
import sys
import subprocess
import argparse
import shutil
import urllib.request
import ssl
import hashlib

VENV_DIR = ".venv"
REQUIREMENTS_BASIC = "requirements.txt"
REQUIREMENTS_DESKTOP = os.path.join("scripts", "ui_designer", "requirements-desktop.txt")
PIP_MIRROR = "https://pypi.tuna.tsinghua.edu.cn/simple"

# Minimum Python version required
MIN_PYTHON_VERSION = (3, 8)

# w64devkit download configuration
W64DEVKIT_VERSION = "2.5.0"
W64DEVKIT_FILENAME = "w64devkit-x64-%s.7z.exe" % W64DEVKIT_VERSION
W64DEVKIT_URL_PRIMARY = (
    "https://github.com/skeeto/w64devkit/releases/download/v%s/%s"
    % (W64DEVKIT_VERSION, W64DEVKIT_FILENAME)
)
# Chinese-friendly mirror
W64DEVKIT_URL_MIRROR = (
    "https://ghfast.top/https://github.com/skeeto/w64devkit/releases/download/v%s/%s"
    % (W64DEVKIT_VERSION, W64DEVKIT_FILENAME)
)
W64DEVKIT_SHA256 = ""  # Fill after verifying; skip check if empty


def get_project_root():
    """Return the project root directory (parent of scripts/)."""
    return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def check_python_version():
    """Ensure Python version meets minimum requirement."""
    ver = sys.version_info
    if ver < MIN_PYTHON_VERSION:
        print("[!!] Python %d.%d 版本过低，需要 Python %d.%d+" %
              (ver.major, ver.minor, MIN_PYTHON_VERSION[0], MIN_PYTHON_VERSION[1]))
        sys.exit(1)
    print("[OK] Python %d.%d.%d" % (ver.major, ver.minor, ver.micro))


def create_venv(project_root):
    """Create virtual environment if it doesn't exist or is broken."""
    venv_path = os.path.join(project_root, VENV_DIR)
    if sys.platform == "win32":
        venv_python = os.path.join(venv_path, "Scripts", "python.exe")
    else:
        venv_python = os.path.join(venv_path, "bin", "python")

    if os.path.exists(venv_python):
        result = subprocess.run([venv_python, "--version"],
                                capture_output=True, text=True)
        if result.returncode == 0:
            print("[OK] .venv 已存在: %s" % result.stdout.strip())
            return venv_python
        else:
            print("[!!] .venv 已损坏，正在重新创建...")
            shutil.rmtree(venv_path, ignore_errors=True)

    print("正在创建虚拟环境 (.venv)...")
    ret = subprocess.run([sys.executable, "-m", "venv", venv_path])
    if ret.returncode != 0:
        print("[!!] 虚拟环境创建失败")
        sys.exit(1)
    print("[OK] .venv 创建成功")
    return venv_python


def pip_install(venv_python, args, desc=""):
    """Run pip install with mirror and given arguments."""
    cmd = [venv_python, "-m", "pip", "install", "-i", PIP_MIRROR] + args
    if desc:
        print("正在安装%s..." % desc)
    ret = subprocess.run(cmd)
    if ret.returncode != 0:
        print("[!!] 安装失败: %s" % " ".join(args))
        print("     请检查网络连接，或尝试配置代理")
        return False
    return True


def install_dependencies(venv_python, project_root, mode):
    """Install pip dependencies into the venv."""
    pip_install(venv_python, ["--upgrade", "pip"], " pip 升级")

    req_basic = os.path.join(project_root, REQUIREMENTS_BASIC)
    if not os.path.exists(req_basic):
        print("[!!] 找不到 %s" % REQUIREMENTS_BASIC)
        return False

    if not pip_install(venv_python, ["-r", req_basic], "基础依赖 (%s)" % REQUIREMENTS_BASIC):
        return False
    print("[OK] 基础依赖安装完成")

    if mode == "2":
        req_desktop = os.path.join(project_root, REQUIREMENTS_DESKTOP)
        if not os.path.exists(req_desktop):
            print("[!!] 找不到 %s" % REQUIREMENTS_DESKTOP)
            return False

        if not pip_install(venv_python, ["-r", req_desktop],
                           "UI Designer 依赖 (%s)" % REQUIREMENTS_DESKTOP):
            return False
        print("[OK] UI Designer 依赖安装完成")

    return True


def verify_imports(venv_python):
    """Verify critical imports work in the venv."""
    packages = ["json5", "PIL", "numpy", "freetype", "elftools"]
    failed = []

    for pkg in packages:
        ret = subprocess.run(
            [venv_python, "-c", "import %s" % pkg],
            capture_output=True, text=True
        )
        if ret.returncode != 0:
            failed.append(pkg)

    if failed:
        print("[!!] 以下包导入失败: %s" % ", ".join(failed))
        return False

    print("[OK] 所有关键包验证通过")
    return True


def _download_file(url, dest_path):
    """Download a file with progress display. Returns True on success."""
    print("  下载地址: %s" % url)
    try:
        # Create SSL context that works in most environments
        ctx = ssl.create_default_context()
        req = urllib.request.Request(url, headers={"User-Agent": "EmbeddedGUI-Setup/1.0"})
        with urllib.request.urlopen(req, context=ctx, timeout=120) as resp:
            total = int(resp.headers.get("Content-Length", 0))
            downloaded = 0
            chunk_size = 256 * 1024  # 256KB chunks

            with open(dest_path, "wb") as f:
                while True:
                    chunk = resp.read(chunk_size)
                    if not chunk:
                        break
                    f.write(chunk)
                    downloaded += len(chunk)
                    if total > 0:
                        pct = downloaded * 100 // total
                        bar = "#" * (pct // 5) + "-" * (20 - pct // 5)
                        print("\r  [%s] %d%% (%d/%d MB)" %
                              (bar, pct, downloaded // 1048576, total // 1048576),
                              end="", flush=True)
            print()  # newline after progress
        return True
    except Exception as e:
        print("\n  下载失败: %s" % e)
        if os.path.exists(dest_path):
            os.remove(dest_path)
        return False


def download_w64devkit(project_root):
    """Download and extract w64devkit toolchain."""
    tools_dir = os.path.join(project_root, "tools")
    devkit_dir = os.path.join(tools_dir, "w64devkit")
    gcc_exe = os.path.join(devkit_dir, "bin", "gcc.exe")

    # Already extracted
    if os.path.exists(gcc_exe):
        result = subprocess.run([gcc_exe, "--version"],
                                capture_output=True, text=True)
        if result.returncode == 0:
            first_line = result.stdout.strip().split("\n")[0]
            print("[OK] w64devkit 已安装: %s" % first_line)
            return True

    os.makedirs(tools_dir, exist_ok=True)
    sfx_path = os.path.join(tools_dir, W64DEVKIT_FILENAME)

    # Download if .7z.exe not present
    if not os.path.exists(sfx_path):
        print("正在下载 w64devkit v%s (约 37 MB)..." % W64DEVKIT_VERSION)
        print()

        # Try mirror first (faster in China), then primary
        urls = [W64DEVKIT_URL_MIRROR, W64DEVKIT_URL_PRIMARY]
        success = False
        for i, url in enumerate(urls):
            if i == 0:
                print("  尝试镜像源...")
            else:
                print("  尝试 GitHub 源...")
            if _download_file(url, sfx_path):
                success = True
                break

        if not success:
            print("[!!] 下载失败。请手动下载:")
            print("     %s" % W64DEVKIT_URL_PRIMARY)
            print("     下载后放到: %s" % sfx_path)
            return False

        # Verify SHA256 if configured
        if W64DEVKIT_SHA256:
            print("  校验文件完整性...")
            sha = hashlib.sha256()
            with open(sfx_path, "rb") as f:
                for block in iter(lambda: f.read(65536), b""):
                    sha.update(block)
            if sha.hexdigest() != W64DEVKIT_SHA256:
                print("[!!] SHA256 校验失败，文件可能损坏")
                os.remove(sfx_path)
                return False

        print("[OK] 下载完成")

    # Extract the self-extracting 7z archive
    print("正在解压 w64devkit...")

    # The .7z.exe is a 7-Zip self-extracting archive
    # -o specifies output directory, -y auto-confirms
    ret = subprocess.run(
        [sfx_path, "-o" + tools_dir, "-y"],
        capture_output=True, text=True
    )

    if ret.returncode != 0 or not os.path.exists(gcc_exe):
        print("[!!] 解压失败")
        if ret.stderr:
            print("     %s" % ret.stderr.strip())
        print("     请手动解压 %s 到 %s" % (sfx_path, tools_dir))
        return False

    # Clean up the archive
    os.remove(sfx_path)
    print("[OK] w64devkit 安装完成")

    # Show version
    result = subprocess.run([gcc_exe, "--version"], capture_output=True, text=True)
    if result.returncode == 0:
        print("     %s" % result.stdout.strip().split("\n")[0])

    return True


def setup_python(project_root, mode):
    """Set up Python venv and install dependencies."""
    print()
    print("=" * 40)
    print("  Python 环境配置")
    print("=" * 40)
    print()

    check_python_version()
    venv_python = create_venv(project_root)

    if not install_dependencies(venv_python, project_root, mode):
        return False

    verify_imports(venv_python)

    print()
    print("提示: 使用前请先激活虚拟环境:")
    if sys.platform == "win32":
        print("  .venv\\Scripts\\activate.bat")
    else:
        print("  source .venv/bin/activate")
    print()
    return True


def main():
    parser = argparse.ArgumentParser(description="EmbeddedGUI environment setup")
    parser.add_argument("--mode", choices=["0", "1", "2"], default="1",
                        help="0=skip Python setup, 1=basic deps, 2=basic + UI Designer")
    parser.add_argument("--install-toolchain", action="store_true",
                        help="Download and install w64devkit (GCC + Make)")
    args = parser.parse_args()

    project_root = get_project_root()
    os.chdir(project_root)

    if args.install_toolchain:
        print()
        print("=" * 40)
        print("  C 工具链配置 (w64devkit)")
        print("=" * 40)
        print()
        if not download_w64devkit(project_root):
            sys.exit(1)

    if args.mode != "0":
        setup_python(project_root, args.mode)


if __name__ == "__main__":
    main()
