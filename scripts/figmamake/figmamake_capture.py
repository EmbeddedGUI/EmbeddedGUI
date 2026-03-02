#!/usr/bin/env python3
"""Stage 1: Playwright-based reference frame capture from Figma Make dev server.

Captures static final frames + animation keyframes (0%/50%/100%) for each page.
Outputs to .eguiproject/reference_frames/.

Usage:
    python scripts/figmamake/figmamake_capture.py \
        --tsx-dir /path/to/figmamake/src \
        --output-dir example/HelloBattery/.eguiproject/reference_frames \
        --width 320 --height 240 \
        --routes "dashboard=/,cells=/battery,temp=/temperature,settings=/settings"
"""

import argparse
import asyncio
import json
import os
import re
import subprocess
import sys
import time
from pathlib import Path


async def capture_pages(tsx_dir: str, output_dir: str, width: int, height: int,
                        routes: dict, boot_page: str = None,
                        anim_duration_ms: int = 1500, dev_port: int = 5173):
    """Capture reference frames from Figma Make dev server using Playwright."""
    try:
        from playwright.async_api import async_playwright
    except ImportError:
        print("ERROR: playwright not installed. Run: pip install playwright && playwright install chromium")
        sys.exit(1)

    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)

    async with async_playwright() as p:
        # Use system Edge on Windows to avoid needing `playwright install`
        launch_opts = {"headless": True}
        if sys.platform == "win32":
            import shutil
            edge_path = shutil.which("msedge")
            if not edge_path:
                for candidate in [
                    os.path.expandvars(r"%ProgramFiles(x86)%\Microsoft\Edge\Application\msedge.exe"),
                    os.path.expandvars(r"%ProgramFiles%\Microsoft\Edge\Application\msedge.exe"),
                ]:
                    if os.path.exists(candidate):
                        edge_path = candidate
                        break
            if edge_path:
                launch_opts["executable_path"] = edge_path
        browser = await p.chromium.launch(**launch_opts)
        context = await browser.new_context(
            viewport={"width": width, "height": height},
            device_scale_factor=1,
        )
        page = await context.new_page()

        base_url = f"http://localhost:{dev_port}"

        for page_name, route in routes.items():
            page_dir = output_path / page_name
            page_dir.mkdir(parents=True, exist_ok=True)

            url = f"{base_url}{route}"
            print(f"  Capturing {page_name} ({url})...")

            # Navigate and wait for initial render
            await page.goto(url, wait_until="networkidle")
            await page.wait_for_timeout(100)

            # Frame 0%: animation start
            await page.screenshot(path=str(page_dir / "frame_0000.png"))

            # Frame 50%: mid-animation
            await page.wait_for_timeout(anim_duration_ms // 2)
            await page.screenshot(path=str(page_dir / "frame_0050.png"))

            # Frame 100%: animation end
            await page.wait_for_timeout(anim_duration_ms // 2)
            await page.screenshot(path=str(page_dir / "frame_0100.png"))

            # Final: static state after all animations settle
            await page.wait_for_timeout(500)
            await page.screenshot(path=str(page_dir / "frame_final.png"))

            print(f"    -> 4 frames saved to {page_dir}")

        # Boot sequence: denser sampling if specified
        if boot_page:
            boot_dir = output_path / "boot"
            boot_dir.mkdir(parents=True, exist_ok=True)
            print(f"  Capturing boot sequence...")

            await page.goto(base_url, wait_until="networkidle")
            frame_idx = 0
            for _ in range(30):  # Up to 3 seconds at 100ms intervals
                await page.screenshot(
                    path=str(boot_dir / f"frame_{frame_idx:04d}.png")
                )
                frame_idx += 1
                await page.wait_for_timeout(100)
            print(f"    -> {frame_idx} boot frames saved")

        await browser.close()


def extract_routes_from_tsx(tsx_dir: str) -> dict:
    """Auto-extract React Router routes from App.tsx."""
    app_tsx = Path(tsx_dir) / "App.tsx"
    if not app_tsx.exists():
        # Try src/App.tsx
        app_tsx = Path(tsx_dir) / "src" / "App.tsx"
    if not app_tsx.exists():
        return {}

    content = app_tsx.read_text(encoding="utf-8")
    routes = {}

    # Match <Route path="/xxx" element={<Component />} />
    pattern = r'<Route\s+path=["\']([^"\']+)["\']\s+element=\{<(\w+)'
    for match in re.finditer(pattern, content):
        path, component = match.groups()
        # Convert component name to snake_case page name
        name = re.sub(r'(?<!^)(?=[A-Z])', '_', component).lower()
        routes[name] = path

    return routes


def parse_routes_arg(routes_str: str) -> dict:
    """Parse --routes 'name1=/path1,name2=/path2' format."""
    routes = {}
    for pair in routes_str.split(","):
        pair = pair.strip()
        if "=" in pair:
            name, path = pair.split("=", 1)
            routes[name.strip()] = path.strip()
    return routes


def start_dev_server(tsx_dir: str, port: int = 5173) -> subprocess.Popen:
    """Start npm dev server and wait for it to be ready."""
    env = os.environ.copy()
    env["PORT"] = str(port)

    npm_cmd = "npm.cmd" if sys.platform == "win32" else "npm"
    proc = subprocess.Popen(
        [npm_cmd, "run", "dev", "--", "--port", str(port)],
        cwd=tsx_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        env=env,
        text=True,
        encoding="utf-8",
        errors="replace",
    )

    # Wait for server to be ready (look for "Local:" in output)
    start = time.time()
    while time.time() - start < 30:
        line = proc.stdout.readline()
        if not line:
            break
        if "Local:" in line or f":{port}" in line:
            print(f"  Dev server ready on port {port}")
            return proc
        time.sleep(0.1)

    print("WARNING: Dev server may not be ready (timeout)")
    return proc


def main():
    parser = argparse.ArgumentParser(description="Capture reference frames from Figma Make project")
    parser.add_argument("--tsx-dir", required=True, help="Path to Figma Make TSX source directory")
    parser.add_argument("--output-dir", required=True, help="Output directory for reference frames")
    parser.add_argument("--width", type=int, default=320, help="Viewport width")
    parser.add_argument("--height", type=int, default=240, help="Viewport height")
    parser.add_argument("--routes", default=None, help="Page routes: 'name1=/path1,name2=/path2'")
    parser.add_argument("--boot", default=None, help="Boot page name for dense capture")
    parser.add_argument("--anim-duration", type=int, default=1500, help="Expected animation duration (ms)")
    parser.add_argument("--port", type=int, default=5173, help="Dev server port")
    parser.add_argument("--no-server", action="store_true", help="Skip starting dev server (already running)")
    args = parser.parse_args()

    # Resolve routes
    if args.routes:
        routes = parse_routes_arg(args.routes)
    else:
        routes = extract_routes_from_tsx(args.tsx_dir)
        if not routes:
            print("ERROR: No routes found. Use --routes to specify manually.")
            sys.exit(1)

    print(f"Routes: {json.dumps(routes, indent=2)}")

    # Start dev server if needed
    server_proc = None
    if not args.no_server:
        print("Starting dev server...")
        server_proc = start_dev_server(args.tsx_dir, args.port)

    try:
        print(f"Capturing {len(routes)} pages at {args.width}x{args.height}...")
        asyncio.run(capture_pages(
            tsx_dir=args.tsx_dir,
            output_dir=args.output_dir,
            width=args.width,
            height=args.height,
            routes=routes,
            boot_page=args.boot,
            anim_duration_ms=args.anim_duration,
            dev_port=args.port,
        ))
        print("Capture complete!")
    finally:
        if server_proc:
            server_proc.terminate()
            server_proc.wait(timeout=5)


if __name__ == "__main__":
    main()
