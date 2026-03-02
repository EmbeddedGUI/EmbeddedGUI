#!/usr/bin/env python3
"""Build all EmbeddedGUI WASM demos and prepare web deployment directory.

Usage:
    python scripts/wasm_build_demos.py [--emsdk-path PATH] [--output-dir DIR] [--app APP]

Optimization: per-app OBJDIR avoids recompiling shared core library.
HelloBasic sub-apps share OBJDIR, so core is compiled once and reused.

Works on both Windows (local dev) and Linux (CI).
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
import time
from concurrent.futures import ProcessPoolExecutor, as_completed


def get_example_list():
    """Scan example/ directory for app names."""
    path = 'example'
    return sorted([f for f in os.listdir(path)
                   if os.path.isdir(os.path.join(path, f))])


def get_example_basic_list():
    """Scan example/HelloBasic/ for sub-app names."""
    path = 'example/HelloBasic'
    return sorted([f for f in os.listdir(path)
                   if os.path.isdir(os.path.join(path, f))])


def run_cmd(cmd, cwd=None):
    """Run a shell command, return success status."""
    result = subprocess.run(cmd, shell=True, cwd=cwd,
                            capture_output=True, text=True)
    if result.returncode != 0:
        return False, result.stderr
    return True, ""


def build_demo(root_dir, app, app_sub, emsdk_path, output_dir):
    """Build a single demo and copy output to web/demos/.

    Uses per-app OBJDIR (set in Makefile) so no make clean is needed.
    """
    if app_sub:
        demo_name = f"{app}_{app_sub}"
        make_extra = f"APP_SUB={app_sub}"
    else:
        demo_name = app
        make_extra = ""

    # Generate resources (may fail if no resources needed)
    res_cmd = f"make resource APP={app} {make_extra}".strip()
    run_cmd(res_cmd, cwd=root_dir)

    # Build (no make clean - per-app OBJDIR handles isolation)
    emsdk_arg = f"EMSDK_PATH={emsdk_path}" if emsdk_path else ""
    build_cmd = f"make -j all APP={app} {make_extra} PORT=emscripten {emsdk_arg}".strip()
    ok, stderr = run_cmd(build_cmd, cwd=root_dir)
    if not ok:
        err_lines = stderr.strip().split('\n')[-3:] if stderr else []
        return {"name": demo_name, "error": "\n".join(err_lines)}

    # Copy output - keep original filenames (emscripten JS references .wasm/.data by APP name)
    demo_dir = os.path.join(output_dir, demo_name)
    os.makedirs(demo_dir, exist_ok=True)

    src_base = os.path.join(root_dir, "output", app)
    for ext in [".html", ".js", ".wasm", ".data"]:
        src = src_base + ext
        if os.path.exists(src):
            dst = os.path.join(demo_dir, app + ext)
            shutil.copy2(src, dst)

    # Copy readme.md into demo directory
    if app_sub:
        readme_src = os.path.join(root_dir, "example", app, app_sub, "readme.md")
    else:
        readme_src = os.path.join(root_dir, "example", app, "readme.md")
    has_doc = False
    if os.path.exists(readme_src):
        shutil.copy2(readme_src, os.path.join(demo_dir, "README.md"))
        has_doc = True

    return {"name": demo_name, "has_doc": has_doc, "app": app}


def build_group_sequential(group, root_dir, emsdk_path, output_dir):
    """Build a group of demos sequentially (for shared-OBJDIR groups like HelloBasic)."""
    results = []
    for app, sub, category in group:
        result = build_demo(root_dir, app, sub, emsdk_path, output_dir)
        result["category"] = category
        results.append(result)
    return results


def main():
    parser = argparse.ArgumentParser(description="Build WASM demos")
    parser.add_argument("--emsdk-path", default=os.environ.get("EMSDK", ""),
                        help="Path to emsdk (default: $EMSDK)")
    parser.add_argument("--output-dir", default="web/demos",
                        help="Output directory (default: web/demos)")
    parser.add_argument("--app", default=None,
                        help="Build single app (e.g. HelloSimple or HelloBasic_button)")
    parser.add_argument("--app-sub", default=None,
                        help="Build single HelloBasic sub-app (e.g. button)")
    parser.add_argument("--clean", action="store_true",
                        help="Clean output directory before building")
    parser.add_argument("--jobs", "-j", type=int, default=1,
                        help="Parallel build jobs for standalone apps (default: 1)")
    args = parser.parse_args()

    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(root_dir)

    output_dir = os.path.join(root_dir, args.output_dir)

    if args.clean:
        # Clean output directory once at start
        out_path = os.path.join(root_dir, "output")
        if os.path.exists(out_path):
            shutil.rmtree(out_path)
            print("Cleaned output/ directory")

    os.makedirs(output_dir, exist_ok=True)

    # Build task list from directory scan
    build_list = []
    if args.app_sub:
        build_list.append(("HelloBasic", args.app_sub, "HelloBasic"))
    elif args.app:
        if args.app.startswith("HelloBasic_"):
            sub = args.app[len("HelloBasic_"):]
            build_list.append(("HelloBasic", sub, "HelloBasic"))
        else:
            build_list.append((args.app, None, "Standalone"))
    else:
        app_sets = get_example_list()
        app_basic_sets = get_example_basic_list()
        for app in app_sets:
            if app == "HelloBasic":
                for sub in app_basic_sets:
                    build_list.append((app, sub, "HelloBasic"))
            else:
                build_list.append((app, None, "Standalone"))

    total = len(build_list)
    start_time = time.time()

    # Split into groups: HelloBasic (must be sequential, shared OBJDIR) and standalone apps
    basic_group = [(a, s, c) for a, s, c in build_list if c == "HelloBasic"]
    standalone_list = [(a, s, c) for a, s, c in build_list if c != "HelloBasic"]

    demos_built = []
    failed = []
    count = 0

    # Build HelloBasic sub-apps sequentially (shared OBJDIR for incremental builds)
    if basic_group:
        print(f"\n--- HelloBasic sub-apps ({len(basic_group)} demos, sequential) ---")
        for app, sub, category in basic_group:
            count += 1
            name = f"{app}_{sub}" if sub else app
            print(f"\n[{count}/{total}] {name}")
            result = build_demo(root_dir, app, sub, args.emsdk_path, output_dir)
            result["category"] = category
            if "error" in result:
                print(f"  FAILED: {result['error']}")
                failed.append(result["name"])
            else:
                print(f"  OK -> {os.path.join(output_dir, result['name'])}")
                entry = {"name": result["name"], "app": result["app"], "category": category}
                if result.get("has_doc"):
                    entry["doc"] = "demos/" + result["name"] + "/README.md"
                demos_built.append(entry)

    # Build standalone apps (can be parallel - each has its own OBJDIR)
    if standalone_list:
        jobs = max(1, min(args.jobs, len(standalone_list)))
        mode = f"parallel x{jobs}" if jobs > 1 else "sequential"
        print(f"\n--- Standalone apps ({len(standalone_list)} demos, {mode}) ---")

        if jobs > 1:
            with ProcessPoolExecutor(max_workers=jobs) as executor:
                future_map = {}
                for app, sub, category in standalone_list:
                    f = executor.submit(build_demo, root_dir, app, sub,
                                        args.emsdk_path, output_dir)
                    future_map[f] = (app, sub, category)

                for future in as_completed(future_map):
                    app, sub, category = future_map[future]
                    count += 1
                    name = f"{app}_{sub}" if sub else app
                    result = future.result()
                    result["category"] = category
                    if "error" in result:
                        print(f"[{count}/{total}] {name} FAILED")
                        failed.append(result["name"])
                    else:
                        print(f"[{count}/{total}] {name} OK")
                        entry = {"name": result["name"], "app": result["app"],
                                 "category": category}
                        if result.get("has_doc"):
                            entry["doc"] = "demos/" + result["name"] + "/README.md"
                        demos_built.append(entry)
        else:
            for app, sub, category in standalone_list:
                count += 1
                name = f"{app}_{sub}" if sub else app
                print(f"\n[{count}/{total}] {name}")
                result = build_demo(root_dir, app, sub, args.emsdk_path, output_dir)
                result["category"] = category
                if "error" in result:
                    print(f"  FAILED: {result['error']}")
                    failed.append(result["name"])
                else:
                    print(f"  OK -> {os.path.join(output_dir, result['name'])}")
                    entry = {"name": result["name"], "app": result["app"],
                             "category": category}
                    if result.get("has_doc"):
                        entry["doc"] = "demos/" + result["name"] + "/README.md"
                    demos_built.append(entry)

    elapsed = time.time() - start_time

    # Generate demos.json for index.html
    json_path = os.path.join(output_dir, "demos.json")

    # In single-app mode, merge into existing demos.json
    if (args.app or args.app_sub) and os.path.exists(json_path):
        with open(json_path, "r", encoding="utf-8") as f:
            existing = json.load(f)
        # Remove old entries for rebuilt demos, then append new
        built_names = {d["name"] for d in demos_built}
        merged = [d for d in existing if d["name"] not in built_names]
        merged.extend(demos_built)
        demos_built = merged

    with open(json_path, "w", encoding="utf-8") as f:
        json.dump(demos_built, f, indent=2)
    print(f"\nGenerated {json_path} with {len(demos_built)} demos")

    print(f"\n{'='*50}")
    print(f"Built: {len(demos_built)}/{total}  Time: {elapsed:.1f}s")
    if failed:
        print(f"Failed: {', '.join(failed)}")

    return 0 if not failed else 1


if __name__ == "__main__":
    sys.exit(main())
