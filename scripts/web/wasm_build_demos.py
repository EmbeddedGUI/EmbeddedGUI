#!/usr/bin/env python3
"""Build all EmbeddedGUI WASM demos and prepare web deployment directory.

Usage:
    python scripts/web/wasm_build_demos.py [--emsdk-path PATH] [--output-dir DIR] [--app APP] [--app-sub SUB]

Optimization: per-app OBJDIR avoids recompiling shared core library.
HelloBasic/HelloVirtual sub-apps share OBJDIR by config hash,
so common framework objects are compiled once and reused across compatible demos.

Works on both Windows (local dev) and Linux (CI).
"""

import argparse
import hashlib
import json
import locale
import os
import re
import shutil
import subprocess
import sys
import time
from concurrent.futures import ProcessPoolExecutor, as_completed
from functools import lru_cache
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
SCRIPTS_ROOT = SCRIPT_DIR.parent
ROOT_DIR = SCRIPTS_ROOT.parent
CUSTOM_WIDGETS_REPO = "https://github.com/EmbeddedGUI/EmbeddedGUI_Widgets"


WASM_SKIP_APPS = {"HelloUnitTest"}
WASM_PRUNE_APPS = {"HelloUnitTest"}
APP_SUB_ROOTS = {
    "HelloBasic": "example/HelloBasic",
    "HelloVirtual": "example/HelloVirtual",
}


def get_example_list():
    """Scan example/ directory for app names."""
    path = 'example'
    return sorted([f for f in os.listdir(path)
                   if os.path.isdir(os.path.join(path, f))
                   and os.path.exists(os.path.join(path, f, 'build.mk'))])


def get_example_sub_list(app):
    """Scan APP_SUB roots for buildable sub-app names."""
    path = APP_SUB_ROOTS.get(app)
    if not path or not os.path.isdir(path):
        return []

    return sorted([f for f in os.listdir(path)
                   if os.path.isdir(os.path.join(path, f))
                   and os.path.exists(os.path.join(path, f, 'app_egui_config.h'))])


def get_example_basic_list():
    """Scan example/HelloBasic/ for sub-app names."""
    return get_example_sub_list("HelloBasic")


def get_example_virtual_list():
    """Scan example/HelloVirtual/ for sub-app names."""
    return get_example_sub_list("HelloVirtual")


def format_demo_name(app, app_sub):
    """Format demo directory name used under web/demos/."""
    if not app_sub:
        return app
    return f"{app}_{app_sub.replace('/', '_')}"


def prune_demo_dirs(output_dir, app_names):
    """Remove previously generated demo directories for skipped app families."""
    output_path = Path(output_dir)
    if not output_path.exists():
        return

    prefixes = tuple(f"{app}_" for app in app_names)
    exact_names = set(app_names)
    for child in output_path.iterdir():
        if not child.is_dir():
            continue
        if child.name in exact_names or child.name.startswith(prefixes):
            shutil.rmtree(child, ignore_errors=True)


def run_cmd(cmd, cwd=None):
    """Run a shell command, return success status."""
    result = subprocess.run(
        cmd,
        shell=True,
        cwd=cwd,
        capture_output=True,
        text=True,
        encoding=locale.getpreferredencoding(False) or "utf-8",
        errors="replace",
    )
    if result.returncode != 0:
        return False, result.stderr
    return True, ""


def parse_define_int(content, macro_name, default):
    """Parse integer value from #define macro."""
    match = re.search(rf'^\s*#\s*define\s+{re.escape(macro_name)}\s+(.+)$', content, re.MULTILINE)
    if not match:
        return default
    value = match.group(1).split('//', 1)[0].strip()
    number_match = re.search(r'\d+', value)
    if not number_match:
        return default
    return int(number_match.group(0))


def get_config_paths(root_dir, app, app_sub=None):
    """Return candidate app config paths from most specific to least specific."""
    config_paths = []
    if app_sub:
        config_paths.append(Path(root_dir) / "example" / app / app_sub / "app_egui_config.h")
    config_paths.append(Path(root_dir) / "example" / app / "app_egui_config.h")
    return config_paths


@lru_cache(maxsize=None)
def get_config_path(root_dir, app, app_sub=None):
    """Resolve the config path used by one app/sub-app."""
    for config_path in get_config_paths(root_dir, app, app_sub):
        if config_path.exists():
            return config_path
    return None


@lru_cache(maxsize=None)
def get_config_hash(root_dir, app, app_sub=None):
    """Hash the effective config so same-config demos can reuse object files."""
    config_path = get_config_path(root_dir, app, app_sub)
    if config_path is None:
        return "default"

    digest = hashlib.sha256(config_path.read_bytes()).hexdigest()
    return digest[:12]


def get_shared_obj_suffix(root_dir, app, app_sub=None):
    """Reuse one OBJDIR per config hash for expensive multi-demo families."""
    if app not in ("HelloBasic", "HelloVirtual"):
        return None
    return f"{app}_cfg_{get_config_hash(root_dir, app, app_sub)}"


def get_screen_size(root_dir, app, app_sub=None):
    """Read app screen size from the most specific app_egui_config.h."""
    config_path = get_config_path(root_dir, app, app_sub)
    if config_path is not None:
        with open(config_path, "r", encoding="utf-8") as f:
            content = f.read()

        width = parse_define_int(content, "EGUI_CONFIG_SCEEN_WIDTH", 240)
        height = parse_define_int(content, "EGUI_CONFIG_SCEEN_HEIGHT", 320)
        return width, height

    return 240, 320


def make_demo_entry(root_dir, result, category):
    """Create one demos.json entry with screen metadata."""
    width, height = get_screen_size(root_dir, result["app"], result.get("app_sub"))
    entry = {
        "name": result["name"],
        "app": result["app"],
        "category": category,
        "width": width,
        "height": height,
    }
    if result.get("app_sub"):
        entry["appSub"] = result["app_sub"]
    if result.get("docPath"):
        entry["doc"] = result["docPath"]
    elif result.get("has_doc"):
        entry["doc"] = "demos/" + result["name"] + "/README.md"
    return entry


def get_auto_make_jobs(concurrent_builds=1):
    """Choose a balanced make -j value for the current machine."""
    cpu_count = os.cpu_count() or 1
    return max(1, cpu_count // max(1, concurrent_builds))


def get_auto_standalone_jobs(standalone_count):
    """Choose a conservative default for standalone app parallelism."""
    if standalone_count <= 1:
        return 1
    cpu_count = os.cpu_count() or 1
    return max(1, min(standalone_count, 4, cpu_count // 8 or 1))


def get_auto_shared_obj_jobs(case_count):
    """Choose a slightly higher default once common objects are already warmed."""
    if case_count <= 1:
        return 1
    cpu_count = os.cpu_count() or 1
    return max(1, min(case_count, 8, cpu_count // 4 or 1))


def get_isolated_build_output_path(root_dir, demo_name):
    """Return a per-demo build output directory used to isolate final artifacts."""
    return str(Path(root_dir) / "output" / "wasm_tmp" / demo_name)


def build_shared_family(group, family_name, root_dir, emsdk_path, output_dir, total, count, demos_built, failed, make_jobs, requested_jobs):
    """Build a multi-demo family that can share objects but needs isolated final outputs."""
    if not group:
        return count

    family_jobs = get_auto_shared_obj_jobs(len(group)) if requested_jobs <= 0 else max(1, min(requested_jobs, len(group)))
    shared_objroot_path = str(Path(root_dir) / "output")
    groups_by_suffix = {}
    for app, sub, category in group:
        suffix = get_shared_obj_suffix(root_dir, app, sub) or format_demo_name(app, sub)
        groups_by_suffix.setdefault(suffix, []).append((app, sub, category))

    mode = f"warmup + parallel x{family_jobs}" if family_jobs > 1 else "sequential"
    print(f"\n--- {family_name} ({len(group)} demos, {mode}, make -j{make_jobs}) ---", flush=True)

    if family_jobs <= 1:
        for app, sub, category in group:
            count += 1
            name = format_demo_name(app, sub)
            print(f"\n[{count}/{total}] {name}", flush=True)
            result = build_demo(
                root_dir,
                app,
                sub,
                emsdk_path,
                output_dir,
                make_jobs,
                get_shared_obj_suffix(root_dir, app, sub),
            )
            result["category"] = category
            if "error" in result:
                print(f"  FAILED: {result['error']}", flush=True)
                failed.append(result["name"])
            else:
                print(f"  OK -> {os.path.join(output_dir, result['name'])}", flush=True)
                demos_built.append(make_demo_entry(root_dir, result, category))
        return count

    parallel_tasks = []
    for suffix in sorted(groups_by_suffix):
        group_cases = groups_by_suffix[suffix]
        seed_app, seed_sub, seed_category = group_cases[0]
        count += 1
        seed_name = format_demo_name(seed_app, seed_sub)
        print(f"\n[{count}/{total}] {seed_name} (warmup)", flush=True)
        result = build_demo(
            root_dir,
            seed_app,
            seed_sub,
            emsdk_path,
            output_dir,
            make_jobs,
            suffix,
            get_isolated_build_output_path(root_dir, seed_name),
            shared_objroot_path,
        )
        result["category"] = seed_category
        if "error" in result:
            print(f"  FAILED: {result['error']}", flush=True)
            failed.append(result["name"])
            continue

        print(f"  OK -> {os.path.join(output_dir, result['name'])}", flush=True)
        demos_built.append(make_demo_entry(root_dir, result, seed_category))
        for app, sub, category in group_cases[1:]:
            parallel_tasks.append((app, sub, category, suffix))

    if not parallel_tasks:
        return count

    with ProcessPoolExecutor(max_workers=min(family_jobs, len(parallel_tasks))) as executor:
        future_map = {}
        for app, sub, category, suffix in parallel_tasks:
            demo_name = format_demo_name(app, sub)
            future = executor.submit(
                build_demo,
                root_dir,
                app,
                sub,
                emsdk_path,
                output_dir,
                make_jobs,
                suffix,
                get_isolated_build_output_path(root_dir, demo_name),
                shared_objroot_path,
            )
            future_map[future] = (app, sub, category)

        for future in as_completed(future_map):
            app, sub, category = future_map[future]
            count += 1
            name = format_demo_name(app, sub)
            result = future.result()
            result["category"] = category
            if "error" in result:
                print(f"[{count}/{total}] {name} FAILED", flush=True)
                failed.append(result["name"])
            else:
                print(f"[{count}/{total}] {name} OK", flush=True)
                demos_built.append(make_demo_entry(root_dir, result, category))

    return count


def build_demo(root_dir, app, app_sub, emsdk_path, output_dir, make_jobs=1, app_obj_suffix=None, build_output_path=None, objroot_path=None):
    """Build a single demo and copy output to web/demos/.

    Uses per-app OBJDIR (set in Makefile) so no make clean is needed.
    """
    demo_name = format_demo_name(app, app_sub)
    make_extra = f"APP_SUB={app_sub}" if app_sub else ""
    default_build_output = Path(root_dir) / "output"
    effective_build_output = Path(build_output_path) if build_output_path else default_build_output

    if not app_sub:
        objdir = Path(root_dir) / "output" / "obj" / app
        stale_pc_port = objdir / "porting" / "pc"
        if stale_pc_port.exists():
            shutil.rmtree(objdir, ignore_errors=True)

    # Build (no make clean - per-app OBJDIR handles isolation)
    emsdk_arg = f"EMSDK_PATH={emsdk_path}" if emsdk_path else ""
    make_parallel_flag = f"-j{max(1, int(make_jobs))} " if make_jobs and int(make_jobs) > 1 else ""
    obj_suffix_arg = f"APP_OBJ_SUFFIX={app_obj_suffix}" if app_obj_suffix else ""
    output_arg = f"OUTPUT_PATH={effective_build_output.as_posix()}"
    objroot_arg = f"OBJROOT_PATH={Path(objroot_path).as_posix()}" if objroot_path else ""
    build_cmd = f"make {make_parallel_flag}all APP={app} {make_extra} PORT=emscripten {obj_suffix_arg} {output_arg} {objroot_arg} {emsdk_arg}".strip()
    ok, stderr = run_cmd(build_cmd, cwd=root_dir)
    if not ok:
        err_lines = stderr.strip().split('\n')[-3:] if stderr else []
        return {"name": demo_name, "error": "\n".join(err_lines)}

    # Copy output - keep original filenames (emscripten JS references .wasm/.data by APP name)
    demo_dir = os.path.join(output_dir, demo_name)
    os.makedirs(demo_dir, exist_ok=True)

    src_base = str(effective_build_output / app)
    for ext in [".html", ".js", ".wasm", ".data"]:
        src = src_base + ext
        if os.path.exists(src):
            dst = os.path.join(demo_dir, app + ext)
            shutil.copy2(src, dst)

    has_doc = False
    result = {"name": demo_name, "has_doc": has_doc, "app": app, "app_sub": app_sub}

    # Copy readme.md into demo directory
    if app_sub:
        readme_src = os.path.join(root_dir, "example", app, app_sub, "readme.md")
    else:
        readme_src = os.path.join(root_dir, "example", app, "readme.md")
    if os.path.exists(readme_src):
        shutil.copy2(readme_src, os.path.join(demo_dir, "README.md"))
        result["has_doc"] = True
        result["docPath"] = f"demos/{demo_name}/README.md"

    if effective_build_output != default_build_output:
        shutil.rmtree(effective_build_output, ignore_errors=True)

    return result


def build_group_sequential(group, root_dir, emsdk_path, output_dir):
    """Build a group of demos sequentially (for shared-OBJDIR groups like HelloBasic)."""
    results = []
    for app, sub, category in group:
        result = build_demo(root_dir, app, sub, emsdk_path, output_dir)
        result["category"] = category
        results.append(result)
    return results


def get_group_build_list(app_name):
    """Expand grouped demos into build list entries."""
    if app_name == "HelloBasic":
        return [(app_name, sub, "HelloBasic") for sub in get_example_basic_list()]
    if app_name == "HelloVirtual":
        return [(app_name, sub, "Standalone") for sub in get_example_virtual_list()]
    return None


def resolve_requested_builds(app_name, app_sub):
    """Resolve command-line selection into build list entries."""
    if app_sub:
        if app_name == "HelloVirtual":
            return [("HelloVirtual", app_sub, "Standalone")]
        if not app_name and app_sub in get_example_virtual_list():
            return [("HelloVirtual", app_sub, "Standalone")]
        if app_name == "HelloBasic" or not app_name:
            return [("HelloBasic", app_sub, "HelloBasic")]
        return [(app_name, app_sub, "Standalone")]

    if not app_name:
        return None

    group_builds = get_group_build_list(app_name)
    if group_builds is not None:
        return group_builds

    if app_name.startswith("HelloBasic_"):
        sub = app_name[len("HelloBasic_"):]
        return [("HelloBasic", sub, "HelloBasic")]

    if app_name.startswith("HelloVirtual_"):
        sub = app_name[len("HelloVirtual_"):]
        return [("HelloVirtual", sub, "Standalone")]

    return [(app_name, None, "Standalone")]


def main():
    local_emsdk = ROOT_DIR / "tools" / "emsdk"
    default_emsdk_path = ""
    if local_emsdk.exists():
        default_emsdk_path = str(local_emsdk)
    else:
        default_emsdk_path = os.environ.get("EMSDK_PATH") or os.environ.get("EMSDK")

    parser = argparse.ArgumentParser(description="Build WASM demos")
    parser.add_argument("--emsdk-path", default=default_emsdk_path or "",
                        help="Path to emsdk (default: $EMSDK)")
    parser.add_argument("--output-dir", default="web/demos",
                        help="Output directory (default: web/demos)")
    parser.add_argument("--app", default=None,
                        help="Build selected app/group (e.g. HelloSimple, HelloBasic_button, HelloVirtual)")
    parser.add_argument("--app-sub", default=None,
                        help="Build single sub-app (e.g. button, virtual_stage_showcase, or chart/radar_chart)")
    parser.add_argument("--clean", action="store_true",
                        help="Clean output directory before building")
    parser.add_argument("--jobs", "-j", type=int, default=0,
                        help="Parallel build jobs for standalone apps. 0 means auto-detect.")
    parser.add_argument("--make-jobs", type=int, default=0,
                        help="Per-build make parallelism. 0 means auto-detect.")
    args = parser.parse_args()

    root_dir = str(ROOT_DIR)
    os.chdir(root_dir)

    requests_custom_widgets = args.app == "HelloCustomWidgets" or (args.app and args.app.startswith("HelloCustomWidgets_"))
    if requests_custom_widgets and not os.path.isdir(os.path.join(root_dir, "example", "HelloCustomWidgets")):
        print("Error: HelloCustomWidgets has moved to the standalone repository:")
        print("  %s" % CUSTOM_WIDGETS_REPO)
        return 1

    output_dir = os.path.join(root_dir, args.output_dir)

    if args.clean:
        # Clean output directory once at start
        out_path = os.path.join(root_dir, "output")
        if os.path.exists(out_path):
            shutil.rmtree(out_path)
            print("Cleaned output/ directory", flush=True)

    os.makedirs(output_dir, exist_ok=True)

    # Default full-site builds should also prune demo families excluded from demos.json,
    # otherwise old directories remain in web/demos and get packaged accidentally.
    if not args.app and not args.app_sub:
        prune_demo_dirs(output_dir, WASM_PRUNE_APPS)

    # Build task list from directory scan
    build_list = []
    requested_builds = resolve_requested_builds(args.app, args.app_sub)
    if requested_builds:
        build_list.extend(requested_builds)
    else:
        app_sets = get_example_list()
        for app in app_sets:
            if app in WASM_SKIP_APPS:
                continue
            group_builds = get_group_build_list(app)
            if group_builds is not None:
                build_list.extend(group_builds)
                continue
            build_list.append((app, None, "Standalone"))

    total = len(build_list)
    start_time = time.time()

    # Split into groups: multi-sub-app families stay sequential, standalone apps may go parallel
    basic_group = [(a, s, c) for a, s, c in build_list if a == "HelloBasic"]
    virtual_group = [(a, s, c) for a, s, c in build_list if a == "HelloVirtual"]
    standalone_list = [(a, s, c) for a, s, c in build_list if a not in ("HelloBasic", "HelloVirtual")]

    demos_built = []
    failed = []
    count = 0
    sequential_make_jobs = args.make_jobs if args.make_jobs > 0 else get_auto_make_jobs(1)

    count = build_shared_family(
        basic_group,
        "HelloBasic sub-apps",
        root_dir,
        args.emsdk_path,
        output_dir,
        total,
        count,
        demos_built,
        failed,
        sequential_make_jobs,
        args.jobs,
    )

    count = build_shared_family(
        virtual_group,
        "HelloVirtual",
        root_dir,
        args.emsdk_path,
        output_dir,
        total,
        count,
        demos_built,
        failed,
        sequential_make_jobs,
        args.jobs,
    )

    # Build standalone apps (can be parallel - each has its own OBJDIR)
    if standalone_list:
        jobs = get_auto_standalone_jobs(len(standalone_list)) if args.jobs <= 0 else max(1, min(args.jobs, len(standalone_list)))
        standalone_make_jobs = args.make_jobs if args.make_jobs > 0 else get_auto_make_jobs(jobs)
        mode = f"parallel x{jobs}" if jobs > 1 else "sequential"
        print(f"\n--- Standalone apps ({len(standalone_list)} demos, {mode}, make -j{standalone_make_jobs}) ---", flush=True)

        if jobs > 1:
            with ProcessPoolExecutor(max_workers=jobs) as executor:
                future_map = {}
                for app, sub, category in standalone_list:
                    f = executor.submit(build_demo, root_dir, app, sub,
                                        args.emsdk_path, output_dir, standalone_make_jobs)
                    future_map[f] = (app, sub, category)

                for future in as_completed(future_map):
                    app, sub, category = future_map[future]
                    count += 1
                    name = format_demo_name(app, sub)
                    result = future.result()
                    result["category"] = category
                    if "error" in result:
                        print(f"[{count}/{total}] {name} FAILED", flush=True)
                        failed.append(result["name"])
                    else:
                        print(f"[{count}/{total}] {name} OK", flush=True)
                        demos_built.append(make_demo_entry(root_dir, result, category))
        else:
            for app, sub, category in standalone_list:
                count += 1
                name = format_demo_name(app, sub)
                print(f"\n[{count}/{total}] {name}", flush=True)
                result = build_demo(root_dir, app, sub, args.emsdk_path, output_dir, standalone_make_jobs)
                result["category"] = category
                if "error" in result:
                    print(f"  FAILED: {result['error']}", flush=True)
                    failed.append(result["name"])
                else:
                    print(f"  OK -> {os.path.join(output_dir, result['name'])}", flush=True)
                    demos_built.append(make_demo_entry(root_dir, result, category))

    elapsed = time.time() - start_time

    # Generate demos.json for index.html
    json_path = os.path.join(output_dir, "demos.json")

    # In single-app mode, merge into existing demos.json
    if (args.app or args.app_sub) and os.path.exists(json_path):
        with open(json_path, "r", encoding="utf-8") as f:
            existing = json.load(f)
        # Remove old entries for rebuilt demos, then append new
        built_names = {d["name"] for d in demos_built}
        if args.app and not args.app_sub and get_group_build_list(args.app) is not None:
            merged = [d for d in existing if d.get("app") != args.app]
        else:
            merged = [d for d in existing if d["name"] not in built_names]
        merged.extend(demos_built)
        demos_built = merged

    with open(json_path, "w", encoding="utf-8") as f:
        json.dump(demos_built, f, indent=2)
    print(f"\nGenerated {json_path} with {len(demos_built)} demos", flush=True)

    print(f"\n{'='*50}", flush=True)
    print(f"Built: {len(demos_built)}/{total}  Time: {elapsed:.1f}s", flush=True)
    if failed:
        print(f"Failed: {', '.join(failed)}", flush=True)

    return 0 if not failed else 1


if __name__ == "__main__":
    sys.exit(main())
