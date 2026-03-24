"""End-to-end smoke check for the UI Designer live preview pipeline.

The script creates a temporary external app workspace, generates a minimal
designer project, compiles it through the normal make-based designer path,
starts the headless preview bridge, verifies animated frame updates, and
checks that a touch interaction mutates the rendered result.
"""

from __future__ import annotations

import argparse
import shutil
import sys
import tempfile
import time
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent

if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from ui_designer.engine.compiler import CompilerEngine
from ui_designer.generator.code_generator import generate_all_files_preserved
from ui_designer.model.page import Page
from ui_designer.model.project import Project
from ui_designer.model.widget_model import AnimationModel, BackgroundModel, WidgetModel
from ui_designer.model.widget_registry import WidgetRegistry
from ui_designer.utils.scaffold import (
    make_app_build_mk_content,
    make_app_config_h_content,
    make_empty_resource_config_content,
)


SCREEN_WIDTH = 240
SCREEN_HEIGHT = 240
APP_NAME = "DesignerPreviewSmoke"
PAGE_NAME = "main_page"
STATUS_REGION = (20, 62, 200, 28)
ANIM_REGION = (20, 170, 180, 40)
BUTTON_CENTER = (120, 130)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run an end-to-end UI Designer live preview smoke check."
    )
    parser.add_argument(
        "--keep-temp",
        action="store_true",
        help="Keep the temporary external workspace after the run.",
    )
    parser.add_argument(
        "--work-dir",
        help="Optional parent directory for the temporary workspace.",
        default="",
    )
    return parser.parse_args()


def print_status(ok: bool, message: str) -> None:
    mark = "[OK]" if ok else "[FAIL]"
    print(f"{mark} {message}", flush=True)


def extract_region(frame: bytes, width: int, x: int, y: int, region_width: int, region_height: int) -> bytes:
    """Return RGB888 bytes for a rectangular sub-region."""
    start_x = max(0, x)
    start_y = max(0, y)
    end_x = max(start_x, min(width, x + region_width))
    bytes_per_row = width * 3
    region_row_bytes = (end_x - start_x) * 3
    rows = []
    for row in range(start_y, start_y + max(0, region_height)):
        row_start = row * bytes_per_row + start_x * 3
        row_end = row_start + region_row_bytes
        if row_start >= len(frame):
            break
        rows.append(frame[row_start:row_end])
    return b"".join(rows)


def _write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _scaffold_app_directory(app_dir: Path, app_name: str) -> None:
    app_dir.mkdir(parents=True, exist_ok=True)
    resource_src_dir = app_dir / "resource" / "src"
    resource_src_dir.mkdir(parents=True, exist_ok=True)
    _write_text(app_dir / "build.mk", make_app_build_mk_content(app_name))
    _write_text(app_dir / "app_egui_config.h", make_app_config_h_content(app_name, SCREEN_WIDTH, SCREEN_HEIGHT))
    _write_text(resource_src_dir / "app_resource_config.json", make_empty_resource_config_content())


def build_smoke_project(app_name: str, sdk_root: str, project_dir: str) -> tuple[Project, dict[str, tuple[int, int, int, int] | tuple[int, int]]]:
    """Build the minimal project model used by the smoke test."""
    WidgetRegistry.instance()
    WidgetModel.reset_counter()

    project = Project(screen_width=SCREEN_WIDTH, screen_height=SCREEN_HEIGHT, app_name=app_name)
    project.sdk_root = str(sdk_root)
    project.project_dir = str(project_dir)
    page = project.create_new_page(PAGE_NAME)
    root = page.root_widget
    if root is None:
        raise RuntimeError("Smoke project did not create a root widget")

    root_bg = BackgroundModel()
    root_bg.bg_type = "solid"
    root_bg.color = "EGUI_COLOR_WHITE"
    root_bg.alpha = "EGUI_ALPHA_100"
    root.background = root_bg

    title = WidgetModel("label", name="title_label", x=20, y=24, width=200, height=28)
    title.properties["text"] = "Designer Preview Smoke"
    title.properties["color"] = "EGUI_COLOR_BLACK"

    status = WidgetModel("label", name="status_label", x=20, y=62, width=200, height=28)
    status.properties["text"] = "Status: idle"
    status.properties["color"] = "EGUI_COLOR_BLACK"

    button = WidgetModel("button", name="action_button", x=30, y=104, width=180, height=52)
    button.properties["text"] = "Tap to verify"
    button.properties["color"] = "EGUI_COLOR_BLACK"
    button.on_click = "smoke_on_action_button_click"

    animated_chip = WidgetModel("group", name="animated_chip", x=32, y=176, width=36, height=20)
    chip_bg = BackgroundModel()
    chip_bg.bg_type = "round_rectangle"
    chip_bg.color = "EGUI_COLOR_RED"
    chip_bg.alpha = "EGUI_ALPHA_100"
    chip_bg.radius = 10
    animated_chip.background = chip_bg
    anim = AnimationModel()
    anim.anim_type = "translate"
    anim.duration = 320
    anim.repeat_count = -1
    anim.repeat_mode = "reverse"
    anim.params = {
        "from_x": 0,
        "to_x": 120,
        "from_y": 0,
        "to_y": 0,
    }
    animated_chip.animations.append(anim)

    for widget in (title, status, button, animated_chip):
        root.add_child(widget)

    meta = {
        "status_region": STATUS_REGION,
        "anim_region": ANIM_REGION,
        "button_center": BUTTON_CENTER,
    }
    return project, meta


def build_main_page_user_source(page: Page) -> str:
    """Return the user-owned page source used by the smoke test."""
    prefix = page.c_prefix
    struct_type = page.c_struct_name
    return f"""// {page.name}.c - user logic for the UI Designer smoke preview test
#include "egui.h"
#include <stdlib.h>

#include "uicode.h"
#include "{page.name}.h"

static {struct_type} *s_page = NULL;

void smoke_on_action_button_click(egui_view_t *self)
{{
    EGUI_UNUSED(self);
    if (s_page == NULL)
        return;

    egui_view_label_set_text((egui_view_t *)&s_page->status_label, "Status: click ok");
    egui_view_label_set_text((egui_view_t *)&s_page->action_button, "Verified");
}}

static void {prefix}_on_open(egui_page_base_t *self)
{{
    {struct_type} *local = ({struct_type} *)self;
    egui_page_base_on_open(self);
    {prefix}_layout_init(self);
    s_page = local;
}}

static void {prefix}_on_close(egui_page_base_t *self)
{{
    {struct_type} *local = ({struct_type} *)self;
    if (s_page == local)
        s_page = NULL;
    egui_page_base_on_close(self);
}}

static void {prefix}_on_key_pressed(egui_page_base_t *self, uint16_t keycode)
{{
    EGUI_UNUSED(self);
    EGUI_UNUSED(keycode);
}}

static const egui_page_base_api_t EGUI_VIEW_API_TABLE_NAME({struct_type}) = {{
    .on_open = {prefix}_on_open,
    .on_close = {prefix}_on_close,
    .on_key_pressed = {prefix}_on_key_pressed,
}};

void {prefix}_init(egui_page_base_t *self)
{{
    egui_page_base_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME({struct_type});
    egui_page_base_set_name(self, "{page.name}");
}}
"""


def _assert_frame(frame: bytes | None, label: str) -> bytes:
    if frame is None:
        raise RuntimeError(f"{label}: no frame returned from preview bridge")
    expected = SCREEN_WIDTH * SCREEN_HEIGHT * 3
    if len(frame) != expected:
        raise RuntimeError(f"{label}: invalid frame size {len(frame)} != {expected}")
    return frame


def _wait_for_region_change(
    compiler: CompilerEngine,
    baseline_region: bytes,
    region: tuple[int, int, int, int],
    *,
    timeout_s: float,
    poll_interval_s: float,
) -> bytes:
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        time.sleep(poll_interval_s)
        frame = _assert_frame(compiler.get_frame(), "preview refresh")
        current_region = extract_region(frame, SCREEN_WIDTH, *region)
        if current_region != baseline_region:
            return frame
    raise RuntimeError("timed out waiting for a rendered region change")


def _perform_click(compiler: CompilerEngine, x: int, y: int) -> None:
    compiler.inject_touch(0x01, x, y)
    _assert_frame(compiler.get_frame(), "button press")
    time.sleep(0.05)
    compiler.inject_touch(0x02, x, y)
    _assert_frame(compiler.get_frame(), "button release")


def run_smoke(work_dir: str = "", keep_temp: bool = False) -> int:
    project_parent = Path(work_dir).resolve() if work_dir else PROJECT_ROOT.parent
    project_parent.mkdir(parents=True, exist_ok=True)
    temp_root = Path(tempfile.mkdtemp(prefix="ui_designer_preview_smoke_", dir=str(project_parent)))
    app_dir = temp_root / APP_NAME
    compiler: CompilerEngine | None = None
    preserve_workspace = keep_temp

    try:
        _scaffold_app_directory(app_dir, APP_NAME)
        project, meta = build_smoke_project(APP_NAME, str(PROJECT_ROOT), str(app_dir))
        project.save(str(app_dir))
        loaded = Project.load(str(app_dir / f"{APP_NAME}.egui"))
        if loaded.sdk_root != str(PROJECT_ROOT):
            raise RuntimeError("saved project did not restore sdk_root correctly")
        print_status(True, f"created external workspace at {app_dir}")

        page = project.get_startup_page()
        if page is None:
            raise RuntimeError("smoke project has no startup page")

        files = generate_all_files_preserved(project, str(app_dir), backup=False)
        files[f"{PAGE_NAME}.c"] = build_main_page_user_source(page)

        compiler = CompilerEngine(str(PROJECT_ROOT), str(app_dir), APP_NAME)
        compiler.set_screen_size(SCREEN_WIDTH, SCREEN_HEIGHT)
        if not compiler.can_build():
            raise RuntimeError(compiler.get_build_error() or "preview compiler is not buildable")

        written = compiler.write_project_files(files)
        print_status(True, f"generated {len(written)} source files")

        success, output = compiler.compile()
        if not success:
            raise RuntimeError(f"designer compile failed:\n{output}")
        print_status(True, "compiled preview app")

        ok, message, _ = compiler._copy_and_start()
        if not ok:
            raise RuntimeError(message)
        ready, err = compiler.validate_preview()
        if not ready:
            raise RuntimeError(err)
        print_status(True, "started headless preview bridge")

        frame0 = _assert_frame(compiler.get_frame(), "initial frame")
        print_status(True, "fetched initial frame")

        anim_region = extract_region(frame0, SCREEN_WIDTH, *meta["anim_region"])
        _wait_for_region_change(
            compiler,
            anim_region,
            meta["anim_region"],
            timeout_s=1.5,
            poll_interval_s=0.15,
        )
        print_status(True, "animation region updated")

        pre_click_frame = _assert_frame(compiler.get_frame(), "pre-click frame")
        status_region = extract_region(pre_click_frame, SCREEN_WIDTH, *meta["status_region"])
        click_x, click_y = meta["button_center"]
        _perform_click(compiler, click_x, click_y)
        _wait_for_region_change(
            compiler,
            status_region,
            meta["status_region"],
            timeout_s=1.0,
            poll_interval_s=0.08,
        )
        print_status(True, "touch interaction updated the status label")
        print_status(True, "UI Designer live preview smoke passed")
        return 0
    except Exception as exc:
        preserve_workspace = True
        print_status(False, str(exc))
        return 1
    finally:
        if compiler is not None:
            compiler.cleanup()
        if preserve_workspace:
            print(f"Workspace: {app_dir}", flush=True)
        else:
            shutil.rmtree(temp_root, ignore_errors=True)


def main() -> int:
    args = parse_args()
    return run_smoke(work_dir=args.work_dir, keep_temp=args.keep_temp)


if __name__ == "__main__":
    sys.exit(main())
