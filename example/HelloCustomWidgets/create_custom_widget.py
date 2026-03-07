#!/usr/bin/env python3
"""Create a new custom widget from template.

Usage:
    python scripts/create_custom_widget.py --category display --name weather_icon
"""

import argparse
import os
import sys

VALID_CATEGORIES = [
    "input", "display", "layout", "chart",
    "navigation", "feedback", "media", "decoration"
]

HEADER_TEMPLATE = '''#ifndef _EGUI_VIEW_{NAME_UPPER}_H_
#define _EGUI_VIEW_{NAME_UPPER}_H_

#include "egui.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {{
#endif

typedef struct egui_view_{name} egui_view_{name}_t;
struct egui_view_{name}
{{
    egui_view_t base;
    // TODO: add widget fields
}};

void egui_view_{name}_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}}
#endif

#endif /* _EGUI_VIEW_{NAME_UPPER}_H_ */
'''

SOURCE_TEMPLATE = '''#include <stdlib.h>
#include "egui_view_{name}.h"

void egui_view_{name}_init(egui_view_t *self)
{{
    EGUI_LOCAL_INIT(egui_view_{name}_t);

    // Init base view
    egui_view_init(self);

    // TODO: initialize widget
}}
'''

TEST_TEMPLATE = '''#include "egui.h"
#include <stdlib.h>
#include "uicode.h"
#include "egui_view_{name}.h"

static egui_view_{name}_t widget;

void test_init_ui(void)
{{
    egui_view_{name}_init(EGUI_VIEW_OF(&widget));
    egui_view_set_size(EGUI_VIEW_OF(&widget), 160, 40);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&widget));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{{
    return false;
}}
#endif
'''


def main():
    parser = argparse.ArgumentParser(description="Create a new custom widget")
    parser.add_argument("--category", required=True, choices=VALID_CATEGORIES,
                        help="Widget category")
    parser.add_argument("--name", required=True,
                        help="Widget name (snake_case, e.g. weather_icon)")
    args = parser.parse_args()

    name = args.name
    name_upper = name.upper()
    category = args.category

    widget_dir = os.path.join("example", "HelloCustomWidgets", category, name)

    if os.path.exists(widget_dir):
        print(f"Error: {widget_dir} already exists!")
        return 1

    os.makedirs(widget_dir, exist_ok=True)

    with open(os.path.join(widget_dir, f"egui_view_{name}.h"), "w", encoding="utf-8") as f:
        f.write(HEADER_TEMPLATE.format(name=name, NAME_UPPER=name_upper))

    with open(os.path.join(widget_dir, f"egui_view_{name}.c"), "w", encoding="utf-8") as f:
        f.write(SOURCE_TEMPLATE.format(name=name))

    with open(os.path.join(widget_dir, "test.c"), "w", encoding="utf-8") as f:
        f.write(TEST_TEMPLATE.format(name=name))

    print(f"Created widget: {widget_dir}/")
    print(f"  egui_view_{name}.h")
    print(f"  egui_view_{name}.c")
    print(f"  test.c")
    print(f"\nBuild: make all APP=HelloCustomWidgets APP_SUB={category}/{name} PORT=pc")
    return 0


if __name__ == "__main__":
    sys.exit(main())
