# -*- mode: python ; coding: utf-8 -*-
"""PyInstaller spec for EmbeddedGUI Designer.

Build command:
    pyinstaller scripts/ui_designer/ui_designer.spec --distpath dist --workpath build/pyinstaller --clean
"""
import os
import sys
from PyInstaller.utils.hooks import collect_all

block_cipher = None

# Paths – SPECPATH is the directory containing this .spec file
ui_designer_dir = os.path.abspath(SPECPATH)          # scripts/ui_designer/
scripts_dir = os.path.dirname(ui_designer_dir)       # scripts/

# Collect custom_widgets/*.py plugins (dynamically loaded by WidgetRegistry)
custom_widgets_dir = os.path.join(ui_designer_dir, 'custom_widgets')
custom_widgets_data = []
if os.path.isdir(custom_widgets_dir):
    for f in os.listdir(custom_widgets_dir):
        if f.endswith('.py'):
            src = os.path.join(custom_widgets_dir, f)
            custom_widgets_data.append((src, os.path.join('ui_designer', 'custom_widgets')))

# Collect qfluentwidgets resources (QSS, icons, etc.)
qfw_datas, qfw_binaries, qfw_hiddenimports = collect_all('qfluentwidgets')

a = Analysis(
    [os.path.join(ui_designer_dir, 'main.py')],
    pathex=[scripts_dir],
    binaries=qfw_binaries,
    datas=custom_widgets_data + qfw_datas,
    hiddenimports=[
        'PyQt5',
        'PyQt5.QtWidgets',
        'PyQt5.QtCore',
        'PyQt5.QtGui',
        'PyQt5.sip',
        'json5',
        'numpy',
        'PIL',
        'freetype',
        'elftools',
    ] + qfw_hiddenimports,
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=['tkinter', 'matplotlib', 'scipy'],
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=block_cipher,
    noarchive=False,
)

pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)

exe = EXE(
    pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name='EmbeddedGUI-Designer',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    console=False,
)

coll = COLLECT(
    exe,
    a.binaries,
    a.zipfiles,
    a.datas,
    strip=False,
    upx=True,
    upx_exclude=[],
    name='EmbeddedGUI-Designer',
)
