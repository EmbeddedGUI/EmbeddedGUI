"""Scaffold utilities for creating new EGUI app project files.

These helpers generate content for project scaffolding (build.mk,
app_egui_config.h, etc.) without any Qt dependencies so they can be
used both by the Designer UI and by unit tests.
"""

import json


def make_app_build_mk_content(app_name):
    """Return the build.mk content string for a new EGUI app.

    Uses directory-based EGUI_CODE_SRC/EGUI_CODE_INCLUDE (no -I prefix,
    no backslash continuations) so the Makefile patsubst can add -I correctly.

    Correct form:
        EGUI_CODE_INCLUDE += $(EGUI_APP_PATH)

    Wrong form (old bug - -I is applied by Makefile's patsubst automatically):
        EGUI_CODE_INCLUDE += -I$(EGUI_APP_PATH)
    """
    lines = [
        f"# Build configuration for {app_name}",
        "",
        "EGUI_CODE_SRC\t\t+= $(EGUI_APP_PATH)",
        "EGUI_CODE_SRC\t\t+= $(EGUI_APP_PATH)/resource",
        "EGUI_CODE_SRC\t\t+= $(EGUI_APP_PATH)/resource/img",
        "EGUI_CODE_SRC\t\t+= $(EGUI_APP_PATH)/resource/font",
        "",
        "EGUI_CODE_INCLUDE\t+= $(EGUI_APP_PATH)",
        "EGUI_CODE_INCLUDE\t+= $(EGUI_APP_PATH)/resource",
        "EGUI_CODE_INCLUDE\t+= $(EGUI_APP_PATH)/resource/img",
        "EGUI_CODE_INCLUDE\t+= $(EGUI_APP_PATH)/resource/font",
        "",
    ]
    return "\n".join(lines)


def make_app_config_h_content(app_name, screen_width=240, screen_height=320):
    """Return the app_egui_config.h content for a new EGUI app."""
    pfb_w = screen_width // 8
    pfb_h = screen_height // 8
    lines = [
        f"#ifndef _APP_EGUI_CONFIG_H_",
        f"#define _APP_EGUI_CONFIG_H_",
        f"",
        f"/* Configuration for {app_name} */",
        f"",
        f"#define EGUI_CONFIG_SCEEN_WIDTH  {screen_width}",
        f"#define EGUI_CONFIG_SCEEN_HEIGHT {screen_height}",
        f"",
        f"#define EGUI_CONFIG_PFB_WIDTH  ({screen_width} / 8)",
        f"#define EGUI_CONFIG_PFB_HEIGHT ({screen_height} / 8)",
        f"",
        f"#define EGUI_CONFIG_COLOR_DEPTH 16",
        f"",
        f"#endif /* _APP_EGUI_CONFIG_H_ */",
        f"",
    ]
    return "\n".join(lines)


def make_empty_resource_config_content():
    """Return the default ``app_resource_config.json`` content."""
    return json.dumps({"img": [], "font": []}, indent=4, ensure_ascii=False) + "\n"
