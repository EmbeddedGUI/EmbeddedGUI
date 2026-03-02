#!/usr/bin/env python3
"""Render HTML design mockup to PNG using Playwright headless browser.

Usage:
    python scripts/figmamake/figma_design_render.py --html design.html --width 320 --height 480 --output ref.png
"""
import argparse
import sys


def _find_edge():
    """Find Microsoft Edge executable."""
    import os
    import shutil
    edge = shutil.which("msedge")
    if edge:
        return edge
    for path in [
        os.path.expandvars(r"%ProgramFiles(x86)%\Microsoft\Edge\Application\msedge.exe"),
        os.path.expandvars(r"%ProgramFiles%\Microsoft\Edge\Application\msedge.exe"),
    ]:
        if os.path.exists(path):
            return path
    return None


EDGE_PATH = _find_edge()


def render_html_to_png(html_path, width, height, output_path, wait_ms=1000):
    """Render an HTML file to a PNG screenshot using Playwright + system Edge."""
    from playwright.sync_api import sync_playwright

    with sync_playwright() as p:
        launch_opts = {"headless": True}
        if EDGE_PATH:
            launch_opts["executable_path"] = EDGE_PATH
        browser = p.chromium.launch(**launch_opts)
        page = browser.new_page(
            viewport={"width": width, "height": height},
            device_scale_factor=1,
        )
        page.goto(f"file:///{html_path.replace(chr(92), '/')}")
        page.wait_for_timeout(wait_ms)  # wait for CDN scripts / fonts
        page.screenshot(path=output_path, full_page=False)
        browser.close()
    print(f"Rendered: {output_path} ({width}x{height})")


def main():
    parser = argparse.ArgumentParser(description="Render HTML to PNG via Playwright")
    parser.add_argument("--html", required=True, help="Path to HTML file")
    parser.add_argument("--width", type=int, default=320)
    parser.add_argument("--height", type=int, default=480)
    parser.add_argument("--output", required=True, help="Output PNG path")
    parser.add_argument("--wait", type=int, default=2000, help="Wait ms for CDN load")
    args = parser.parse_args()

    import os
    html_abs = os.path.abspath(args.html)
    if not os.path.exists(html_abs):
        print(f"Error: {html_abs} not found", file=sys.stderr)
        sys.exit(1)

    render_html_to_png(html_abs, args.width, args.height, args.output, args.wait)


if __name__ == "__main__":
    main()
