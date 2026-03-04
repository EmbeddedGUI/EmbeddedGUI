#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
extract_font_text.py - Extract text strings from C source files and
generate per-font text files for the EmbeddedGUI resource pipeline.

Each source string is written as an individual line, which makes the
text files easy to maintain and review.  The resource generator
(app_resource_generate.py) already deduplicates characters, so
duplicates in the text files are harmless.

Usage:
    python scripts/tools/extract_font_text.py --app HelloShowcase
    python scripts/tools/extract_font_text.py --app MyApp --dry-run
    python scripts/tools/extract_font_text.py --app MyApp --overwrite

Font type detection is based on the TTF filename:
    cn   - noto*sc, *hans*, simhei, simsun, *cjk*, *chinese* ...
    icon - material*, *symbol*, *icon*, *fa-*, fontawesome ...
    latin- everything else (skipped; Latin chars are in the default font)

Outputs per-font text files in example/{APP}/resource/src/.
"""

import re
import os
import sys
import json
import argparse


# ---------------------------------------------------------------------------
# Unicode category helpers
# ---------------------------------------------------------------------------

def is_cjk(ch):
    """Return True if the character is in a CJK / Chinese Unicode range."""
    cp = ord(ch)
    return (
        0x4E00 <= cp <= 0x9FFF   or  # CJK Unified Ideographs (most common)
        0x3400 <= cp <= 0x4DBF   or  # CJK Extension A
        0x20000 <= cp <= 0x2A6DF or  # CJK Extension B
        0x2A700 <= cp <= 0x2B73F or  # CJK Extension C
        0x2B740 <= cp <= 0x2B81F or  # CJK Extension D
        0xF900 <= cp <= 0xFAFF   or  # CJK Compatibility Ideographs
        0x2F800 <= cp <= 0x2FA1F or  # CJK Compatibility Ideographs Suppl.
        0x3000 <= cp <= 0x303F   or  # CJK Symbols and Punctuation
        0xFF00 <= cp <= 0xFFEF       # Fullwidth & Halfwidth Forms
    )


def is_pua(ch):
    """Return True if the character is in the BMP Private Use Area (icons)."""
    cp = ord(ch)
    return 0xE000 <= cp <= 0xF8FF


def has_cjk(text):
    return any(is_cjk(c) for c in text)


def has_pua(text):
    return any(is_pua(c) for c in text)


# ---------------------------------------------------------------------------
# C string literal decoder
# ---------------------------------------------------------------------------

def decode_c_string(raw):
    """
    Decode the raw content between C string quotes (\\-escapes still
    present as text) into a Python str.

    Handles: \\n \\t \\r \\\\ \\" \\x NN  \\u NNNN  \\U NNNNNNNN
    Multi-byte \\xNN sequences are assembled as UTF-8 bytes before
    decoding so that Material-icon hex escapes work correctly.
    """
    byte_list = []
    i = 0
    while i < len(raw):
        if raw[i] == '\\' and i + 1 < len(raw):
            esc = raw[i + 1]
            if esc == 'n':
                byte_list.append(0x0A); i += 2
            elif esc == 't':
                byte_list.append(0x09); i += 2
            elif esc == 'r':
                byte_list.append(0x0D); i += 2
            elif esc == '\\':
                byte_list.append(0x5C); i += 2
            elif esc == '"':
                byte_list.append(0x22); i += 2
            elif esc == "'":
                byte_list.append(0x27); i += 2
            elif esc == '0':
                byte_list.append(0x00); i += 2
            elif esc == 'x':
                # Collect up to 2 hex digits
                j = i + 2
                hex_str = ''
                while j < len(raw) and len(hex_str) < 2 and raw[j] in '0123456789abcdefABCDEF':
                    hex_str += raw[j]
                    j += 1
                if hex_str:
                    byte_list.append(int(hex_str, 16))
                    i = j
                else:
                    byte_list.append(ord('\\'))
                    i += 1
            elif esc == 'u':
                hex_str = raw[i + 2:i + 6]
                try:
                    byte_list.extend(chr(int(hex_str, 16)).encode('utf-8'))
                    i += 6
                except (ValueError, OverflowError):
                    byte_list.append(ord('\\'))
                    i += 1
            elif esc == 'U':
                hex_str = raw[i + 2:i + 10]
                try:
                    byte_list.extend(chr(int(hex_str, 16)).encode('utf-8'))
                    i += 10
                except (ValueError, OverflowError):
                    byte_list.append(ord('\\'))
                    i += 1
            else:
                byte_list.append(ord(esc))
                i += 2
        else:
            byte_list.extend(raw[i].encode('utf-8', errors='replace'))
            i += 1

    try:
        return bytes(byte_list).decode('utf-8', errors='replace')
    except Exception:
        return ''


# ---------------------------------------------------------------------------
# Source file parsing
# ---------------------------------------------------------------------------

def strip_comments(content):
    """Remove C block and line comments."""
    content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
    content = re.sub(r'//[^\n]*', '', content)
    return content


def extract_all_string_literals(content):
    """Return list of decoded string literals found in stripped C content."""
    results = []
    pattern = re.compile(r'"((?:[^"\\]|\\.)*)"')
    for m in pattern.finditer(content):
        decoded = decode_c_string(m.group(1))
        if decoded:
            results.append(decoded)
    return results


def extract_s_macro_pairs(content):
    """
    Extract (en, cn) pairs from S("en", "cn") bilingual macro calls.
    Returns list of (en_str, cn_str).
    """
    pattern = re.compile(
        r'\bS\s*\(\s*"((?:[^"\\]|\\.)*)"\s*,\s*"((?:[^"\\]|\\.)*)"\s*\)'
    )
    pairs = []
    for m in pattern.finditer(content):
        en = decode_c_string(m.group(1))
        cn = decode_c_string(m.group(2))
        pairs.append((en, cn))
    return pairs

def extract_ternary_cjk_pairs(content):
    """
    Extract both sides of ternary expressions where one side contains CJK.

    Catches patterns like:
        is_chinese ? "EN" : "中文"
        flag       ? "中文" : "EN"

    Both strings must be included in the CN font text file because the same
    widget uses the CN font regardless of which branch is active.
    Returns a list of strings (all sides where the paired string has CJK).
    """
    pattern = re.compile(
        r'\?\s*"((?:[^"\\]|\\.)*)"' + r'\s*:\s*' + r'"((?:[^"\\]|\\.)*)"'
    )
    results = []
    for m in pattern.finditer(content):
        a = decode_c_string(m.group(1))
        b = decode_c_string(m.group(2))
        # Only include both sides when (at least) one side has CJK
        if has_cjk(a) or has_cjk(b):
            if a.strip():
                results.append(a)
            if b.strip():
                results.append(b)
    return results

def extract_cn_array_strings(content):
    """
    Extract strings from static arrays whose name ends with _cn[]:
        static const char *roller_items_cn[] = {"Mon", ...};
    Returns list of strings.
    """
    results = []
    # Match array declaration with _cn suffix
    array_pattern = re.compile(
        r'\w+\s+\w+\s+\*?\w+_cn\s*\[\s*\]\s*=\s*\{([^}]*)\}',
        re.DOTALL
    )
    str_pattern = re.compile(r'"((?:[^"\\]|\\.)*)"')
    for m in array_pattern.finditer(content):
        body = m.group(1)
        for sm in str_pattern.finditer(body):
            decoded = decode_c_string(sm.group(1))
            if decoded.strip():
                results.append(decoded)
    return results


def extract_cn_struct_strings(content):
    """
    Extract .text = "..." strings from structs named with _cn:
        static const egui_view_menu_item_t menu_items_cn[] = {
            {.text = "设置", .sub_page_index = 0xFF},
    """
    results = []
    # Find blocks after variable names ending in _cn
    block_pattern = re.compile(
        r'\w+_cn\s*\[\s*\]\s*=\s*\{(.*?)\}\s*;',
        re.DOTALL
    )
    str_pattern = re.compile(r'"((?:[^"\\]|\\.)*)"')
    for m in block_pattern.finditer(content):
        body = m.group(1)
        for sm in str_pattern.finditer(body):
            decoded = decode_c_string(sm.group(1))
            if decoded.strip() and has_cjk(decoded):
                results.append(decoded)
    return results


def parse_c_file(filepath):
    """
    Parse a single C/H file and return:
        cn_strings  - list of Chinese text strings (per-source-string)
        pua_strings - list of strings containing PUA (icon) characters
        all_strings - all string literals (for catch-all fallback)
    """
    with open(filepath, encoding='utf-8', errors='replace') as f:
        raw = f.read()

    content = strip_comments(raw)

    # All decoded string literals
    all_strs = extract_all_string_literals(content)

    # S() macro Chinese strings (most reliable source)
    s_pairs = extract_s_macro_pairs(content)
    cn_from_macro = [cn for (_, cn) in s_pairs if cn.strip()]

    # Ternary ? "A" : "B" pairs where one side has CJK — include BOTH sides.
    # Rationale: when a widget always uses the CN font but its text alternates
    # between a CJK string and a Latin string (e.g. is_chinese ? "EN" : "中文"),
    # the Latin side also needs to be present in the CN font character set.
    cn_from_ternary = extract_ternary_cjk_pairs(content)

    # _cn[] array strings
    cn_from_arrays = extract_cn_array_strings(content)
    cn_from_structs = extract_cn_struct_strings(content)

    # Catch-all: any string literal that contains CJK chars directly
    # (handles direct Chinese assignments not wrapped in S())
    cn_direct = [s for s in all_strs if has_cjk(s)]

    # Merge, preserving order, removing duplicates
    seen = set()
    cn_strings = []
    for s in cn_from_macro + cn_from_ternary + cn_from_arrays + cn_from_structs + cn_direct:
        clean = s.replace('\n', '').replace('\r', '').replace('\0', '').strip()
        if clean and clean not in seen:
            seen.add(clean)
            cn_strings.append(clean)

    # Icon / PUA strings (from hex escapes or direct PUA chars)
    pua_strings = []
    pua_seen = set()
    for s in all_strs:
        if has_pua(s):
            clean = s.strip()
            if clean and clean not in pua_seen:
                pua_seen.add(clean)
                pua_strings.append(clean)

    return cn_strings, pua_strings, all_strs


# ---------------------------------------------------------------------------
# Font type detection
# ---------------------------------------------------------------------------

CN_FONT_KEYWORDS = [
    'noto', 'sc', 'hans', 'cn', 'chinese', 'cjk', 'simhei', 'simsun',
    'source_han', 'wenquanyi', 'unifont', 'wqy', 'notosans',
]

ICON_FONT_KEYWORDS = [
    'material', 'symbol', 'icon', 'fontawesome', 'fa-', 'ionicon',
    'feather', 'boxicon', 'remixicon',
]


def detect_font_type(font_filename):
    """Guess the font's character domain from its filename."""
    name = font_filename.lower()
    if any(k in name for k in ICON_FONT_KEYWORDS):
        return 'icon'
    if any(k in name for k in CN_FONT_KEYWORDS):
        return 'cn'
    return 'latin'


# ---------------------------------------------------------------------------
# Resource config loader
# ---------------------------------------------------------------------------

def load_resource_config(resource_src_dir):
    config_path = os.path.join(resource_src_dir, 'app_resource_config.json')
    if not os.path.exists(config_path):
        return None
    with open(config_path, encoding='utf-8') as f:
        content = re.sub(r'//[^\n]*', '', f.read())  # strip json5 comments
        return json.loads(content)


# ---------------------------------------------------------------------------
# Source file discovery
# ---------------------------------------------------------------------------

SKIP_DIRS = {'resource', 'output', 'obj', '__pycache__', '.git'}


def find_c_source_files(src_dir):
    """Recursively find *.c and *.h source files, skipping generated dirs."""
    result = []
    for root, dirs, files in os.walk(src_dir):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS]
        for f in files:
            if f.endswith(('.c', '.h')):
                result.append(os.path.join(root, f))
    return result


# ---------------------------------------------------------------------------
# Text file read / merge / write helpers
# ---------------------------------------------------------------------------

def read_text_file(path):
    """Read existing text file, return list of lines (stripped)."""
    if not os.path.exists(path):
        return []
    with open(path, encoding='utf-8') as f:
        return [l.rstrip('\n') for l in f.readlines()]


def chars_in_lines(lines):
    """Collect all non-whitespace characters present in a list of lines."""
    chars = set()
    for line in lines:
        # Expand &#xNNNN; entities
        expanded = re.sub(r'&#x([0-9A-Fa-f]+);', lambda m: chr(int(m.group(1), 16)), line)
        for ch in expanded:
            if not ch.isspace():
                chars.add(ch)
    return chars


def pua_char_to_entity(ch):
    return f'&#x{ord(ch):04X};'


def format_pua_line(text):
    """Format a PUA string as a line of &#xNNNN; entities."""
    return ''.join(pua_char_to_entity(c) if is_pua(c) else c for c in text)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='Extract per-font text files from C source for EmbeddedGUI resource pipeline'
    )
    parser.add_argument('--app', required=True, help='App name, e.g. HelloShowcase')
    parser.add_argument('--src-dir', help='Source directory (default: example/<APP>)')
    parser.add_argument('--resource-dir', help='Resource src dir (default: example/<APP>/resource/src)')
    parser.add_argument('--dry-run', action='store_true', help='Print result without writing files')
    parser.add_argument('--overwrite', action='store_true',
                        help='Overwrite existing text files (default: merge/append new strings)')
    args = parser.parse_args()

    app_name = args.app
    src_dir = args.src_dir or os.path.join('example', app_name)
    resource_src_dir = args.resource_dir or os.path.join('example', app_name, 'resource', 'src')

    # ------------------------------------------------------------------
    # Load font config
    # ------------------------------------------------------------------
    config = load_resource_config(resource_src_dir)
    if config is None:
        print(f'ERROR: app_resource_config.json not found in {resource_src_dir}')
        sys.exit(1)

    font_entries = config.get('font', [])
    if not font_entries:
        print('No font entries in config, nothing to do.')
        return

    # ------------------------------------------------------------------
    # Parse source files
    # ------------------------------------------------------------------
    c_files = find_c_source_files(src_dir)
    print(f'Scanning {len(c_files)} source file(s) in {src_dir} ...')

    all_cn_strings = []
    all_pua_strings = []

    for filepath in sorted(c_files):
        cn, pua, _ = parse_c_file(filepath)
        all_cn_strings.extend(cn)
        all_pua_strings.extend(pua)

    # Deduplicate while preserving insertion order
    def dedup(lst):
        seen = set()
        out = []
        for item in lst:
            if item not in seen:
                seen.add(item)
                out.append(item)
        return out

    all_cn_strings = dedup(all_cn_strings)
    all_pua_strings = dedup(all_pua_strings)

    print(f'  Chinese strings found : {len(all_cn_strings)}')
    print(f'  Icon/PUA strings found: {len(all_pua_strings)}')

    if args.dry_run:
        print('\n[DRY RUN] Chinese strings:')
        for s in all_cn_strings:
            print(f'  {repr(s)}')
        print('\n[DRY RUN] Icon/PUA strings:')
        for s in all_pua_strings:
            print(f'  {repr(s)}')
        return

    # ------------------------------------------------------------------
    # Write text files per font entry
    # ------------------------------------------------------------------
    for entry in font_entries:
        font_file = entry.get('file', '')
        font_name = entry.get('name', font_file)
        pixel_size = entry.get('pixelsize', '?')
        text_file_cfg = entry.get('text', '')
        if not text_file_cfg:
            continue

        # Use the first text file listed (comma-separated list supported by pipeline)
        text_filename = text_file_cfg.split(',')[0].strip()
        output_path = os.path.join(resource_src_dir, text_filename)

        font_type = detect_font_type(font_file)

        print(f'\nFont: {font_name} ({font_file}, {pixel_size}px) -> {text_filename}  [type={font_type}]')

        if font_type == 'cn':
            new_strings = all_cn_strings
        elif font_type == 'icon':
            new_strings = all_pua_strings
        else:
            print(f'  Latin font, skipping (ASCII is in default font).')
            continue

        if not new_strings:
            print(f'  No strings found for this font type, skipping.')
            continue

        # Read existing file to avoid losing manually curated entries
        if args.overwrite:
            existing_lines = []
        else:
            existing_lines = read_text_file(output_path)

        existing_chars = chars_in_lines(existing_lines)
        existing_line_set = set(existing_lines)

        added = 0
        out_lines = list(existing_lines)

        for s in new_strings:
            if font_type == 'cn':
                # Check if all CJK chars in this string are already covered
                needed = set(c for c in s if is_cjk(c)) - existing_chars
                if needed or s not in existing_line_set:
                    out_lines.append(s)
                    existing_chars.update(c for c in s if is_cjk(c))
                    existing_line_set.add(s)
                    added += 1
            elif font_type == 'icon':
                line = format_pua_line(s)
                needed = set(c for c in s if is_pua(c)) - existing_chars
                if needed or line not in existing_line_set:
                    out_lines.append(line)
                    existing_chars.update(c for c in s if is_pua(c))
                    existing_line_set.add(line)
                    added += 1

        if added == 0:
            print(f'  All strings already present, no changes needed.')
            continue

        with open(output_path, 'w', encoding='utf-8') as f:
            for line in out_lines:
                f.write(line + '\n')

        total_chars = len(set(c for line in out_lines
                              for c in re.sub(r'&#x([0-9A-Fa-f]+);',
                                              lambda m: chr(int(m.group(1), 16)), line)
                              if (is_cjk(c) if font_type == 'cn' else is_pua(c))))
        print(f'  Written {output_path}')
        print(f'  Lines: {len(out_lines)} ({added} new)  |  Unique chars: {total_chars}')


if __name__ == '__main__':
    main()
