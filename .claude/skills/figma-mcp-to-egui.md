---
name: figma-mcp-to-egui
description: Convert Figma designs to EmbeddedGUI C code via Figma MCP tools and XML pipeline
---

# Figma MCP to EmbeddedGUI Conversion Skill

通过 Figma MCP 工具直接从 Figma 设计稿获取节点数据，转换为 EmbeddedGUI C 代码。

## Prerequisites

- Figma Desktop 已打开目标设计文件
- MCP server 已启用（Figma Desktop → Dev Mode → Inspect → MCP）
- Material Symbols Outlined 字体位于 `scripts/tools/MaterialSymbolsOutlined-Regular.ttf`

## Workflow Overview

```
1. Discover frames   -- MCP get_file → list top-level frames
2. Fetch node tree   -- MCP get_node(node_id) → full subtree JSON
3. Save design screenshot -- MCP get_images → mockup PNG
4. scaffold          -- Script creates .egui project structure
5. figma-mcp         -- Script converts MCP JSON to XML layout
6. Review XML        -- AI reviews and fixes XML if needed
7. generate-code     -- Script converts XML to C code
8. gen-resource      -- Script generates C image/font arrays
9. verify + compare  -- Build, runtime check, visual comparison
```

## Step 1: Discover Frames

Use MCP tool to get the file structure:

```
MCP call: get_file(file_key)
```

From the response, identify top-level frames (pages). Each frame with `type: "FRAME"` at the top level of a page becomes an EGUI page.

Record:
- `file_key` — Figma file identifier
- `node_id` — Target frame's node ID (e.g., "123:456")
- Frame dimensions (width × height)

## Step 2: Fetch Node Tree

```
MCP call: get_node(file_key, node_id)
```

Save the JSON response to a file:
```bash
# Save MCP response to file
# ensure directory exists: example/MyApp/.eguiproject/tmp/
echo '<MCP JSON>' > example/MyApp/.eguiproject/tmp/figma_node.json
```

## Step 3: Save Design Screenshot

Export the design frame as PNG for later comparison:

```
MCP call: get_images(file_key, node_ids=[node_id], format="png", scale=2)
```

Save to: `example/{AppName}/.eguiproject/mockup/design_screenshot.png`

## Step 4: Scaffold Project

```bash
python scripts/html2egui_helper.py scaffold --app MyApp --width 320 --height 480
```

## Step 5: Convert MCP JSON to XML

```bash
python scripts/html2egui_helper.py figma-mcp \
    --input example/MyApp/.eguiproject/tmp/figma_node.json \
    --app MyApp \
    --target 320x480 \
    --page main_page
```

This generates `.eguiproject/layout/main_page.xml`.

## Step 6: Review and Fix XML

AI must review the generated XML and fix common issues:

### Auto-Layout Mapping (automatic)

| Figma Property | EGUI Result |
|---|---|
| `layoutMode: "VERTICAL"` | `<LinearLayout orientation="vertical">` |
| `layoutMode: "HORIZONTAL"` | `<LinearLayout orientation="horizontal">` |
| `layoutWrap: "WRAP"` | `<GridLayout>` |
| `itemSpacing` | child `margin` attribute |

### Common Fixes Needed

1. **Duplicate IDs** — Figma layers with same name produce duplicate `id`. Rename to unique C identifiers.
2. **Missing icons** — Vector nodes output as `<!-- Vector: name -->` comments. Export as PNG manually or via MCP `get_images`.
3. **Font size mismatch** — Verify font references match available builtin fonts (see html-to-egui.md font table).
4. **Label height** — Must be ≥ font line height.
5. **Nested auto-layout** — Verify LinearLayout nesting is correct.

### XML Rules (same as html-to-egui.md)

1. Root is `<Page>`, direct child is `<Group id="root">` spanning full screen
2. All widgets MUST have `id`, `x`, `y`, `width`, `height`
3. `id` must be unique valid C identifier
4. Font references use `&amp;` (XML escape)
5. Alpha-only images MUST set `image_color` and `image_color_alpha`
6. When image size ≠ resource PNG size, set `image_resize="true"`
7. Label height ≥ font line height
8. Card children auto-positioned by `layout_childs`
9. GridLayout/LinearLayout children auto-positioned with `margin` for gaps
10. Group children use explicit x/y positioning
11. `<Background>` is optional child element, not attribute
12. Colors: `EGUI_COLOR_WHITE`, `EGUI_COLOR_HEX(0xRRGGBB)`

## Step 7-8: Generate Code and Resources

```bash
python scripts/html2egui_helper.py generate-code --app MyApp
python scripts/html2egui_helper.py gen-resource --app MyApp
```

## Step 9: Build, Verify, and Compare

```bash
# Build + runtime check + visual comparison
python scripts/html2egui_helper.py verify --app MyApp \
    --compare-design example/MyApp/.eguiproject/mockup/design_screenshot.png
```

Or separately:
```bash
make all APP=MyApp PORT=pc
python scripts/code_runtime_check.py --app MyApp --timeout 10
python scripts/figmamake/figma_visual_compare.py \
    --design example/MyApp/.eguiproject/mockup/design_screenshot.png \
    --rendered runtime_check_output/MyApp/default/frame_0000.png \
    --output runtime_check_output/MyApp/comparison.png
```

### Interpreting Results

- **≥ 85%** similarity → PASS, high fidelity
- **70-85%** → Review comparison image, fix major differences in XML
- **< 70%** → Significant issues, likely missing widgets or wrong layout

## Iteration Loop

If similarity < 85%:
1. Read the comparison image (`comparison.png`) — 3 panels: Design / Rendered / Diff
2. Identify areas with red/bright pixels in the Diff panel
3. Fix corresponding XML elements (position, size, color, missing widgets)
4. Re-run steps 7-9
5. Repeat until ≥ 85%

## Multi-Page Projects

For Figma files with multiple frames:

```bash
# Step 1: Scaffold with multiple pages
python scripts/html2egui_helper.py scaffold --app MyApp --width 320 --height 480

# Step 2: Convert each frame separately
python scripts/html2egui_helper.py figma-mcp --input frame1.json --app MyApp --page main_page
python scripts/html2egui_helper.py figma-mcp --input frame2.json --app MyApp --page settings_page

# Step 3: Update .egui project file to include all pages
# Step 4: Add onClick navigation between pages
# Step 5: generate-code + gen-resource + verify
```

Navigation between pages uses `onClick` attribute:
```xml
<Label id="nav_main" x="0" y="8" width="80" height="20"
       text="MAIN" font_builtin="&amp;egui_res_font_montserrat_10_4"
       color="EGUI_COLOR_WHITE" alpha="EGUI_ALPHA_30"
       align_type="EGUI_ALIGN_HCENTER" onClick="on_nav_main_click" />
```

Click callbacks should be implemented in `uicode.c` (shared across all pages):
```c
// Navigation bar click callbacks (shared across all pages)
void on_nav_main_click(egui_view_t *self)
{
    uicode_switch_page(PAGE_DASHBOARD);
}
```

Add a duplicate page guard in `uicode_switch_page()` to avoid unnecessary re-init:
```c
void uicode_switch_page(int page_index)
{
    if (page_index == current_index && current_page)
        return; // Already on this page
    ...
}
```

### Icon Image Elements

Alpha-only icons require `image_color` and `image_color_alpha`:
```xml
<Image id="icon_alarm" x="8" y="6" width="16" height="16"
       image_file="icon_notifications.png" image_format="alpha" image_alpha="4"
       image_color="EGUI_COLOR_HEX(0x00F3FF)" image_color_alpha="EGUI_ALPHA_100"
       image_resize="true" />
```
