"""Auto-fixer engine for EGUI UI Designer.

Detects and automatically fixes common layout issues:
1. Label height < font line height
2. Container overflow (children exceed parent bounds)
3. Overlap detection (warning only)
4. Image resize missing
5. ID conflict (duplicate widget names)
"""

import re


class FixReport:
    """Collects auto-fix results and warnings."""

    def __init__(self):
        self.auto_fixed = []
        self.warnings = []

    def add_fix(self, rule, widget_name, description):
        self.auto_fixed.append({
            "rule": rule,
            "widget": widget_name,
            "description": description,
        })

    def add_warning(self, rule, widget_name, description):
        self.warnings.append({
            "rule": rule,
            "widget": widget_name,
            "description": description,
        })

    def to_dict(self):
        return {
            "auto_fixed": list(self.auto_fixed),
            "warnings": list(self.warnings),
        }


def _extract_font_size(font_expr):
    """Extract pixel size from font expression like '&egui_res_font_montserrat_14_4'."""
    if not font_expr:
        return None
    m = re.search(r'_(\d+)_\d+$', font_expr)
    if m:
        return int(m.group(1))
    return None


def _collect_all_widgets(widgets):
    """Flatten widget tree into a list."""
    result = []
    for w in widgets:
        result.append(w)
        if hasattr(w, "children"):
            result.extend(_collect_all_widgets(w.children))
    return result


def _rects_overlap(x1, y1, w1, h1, x2, y2, w2, h2):
    """Check if two rectangles overlap."""
    if x1 >= x2 + w2 or x2 >= x1 + w1:
        return False
    if y1 >= y2 + h2 or y2 >= y1 + h1:
        return False
    return True


class AutoFixer:
    """Applies auto-fix rules to a list of widgets."""

    def fix_widgets(self, widgets):
        """Run all fix rules on the widget list. Returns FixReport."""
        report = FixReport()
        all_widgets = _collect_all_widgets(widgets)
        self._fix_label_heights(all_widgets, report)
        self._fix_container_overflow(all_widgets, report)
        self._detect_overlaps(all_widgets, report)
        self._fix_image_resize(all_widgets, report)
        self._fix_id_conflicts(all_widgets, report)
        return report

    def _fix_label_heights(self, widgets, report):
        for w in widgets:
            if w.widget_type not in ("label", "dynamic_label", "textblock"):
                continue
            font = w.properties.get("font", "")
            font_size = _extract_font_size(font)
            if font_size and w.height < font_size:
                old_h = w.height
                w.height = font_size + 2  # small padding
                report.add_fix(
                    "label_height", w.name,
                    f"height {old_h} -> {w.height} (font size {font_size})",
                )

    def _fix_container_overflow(self, widgets, report):
        for w in widgets:
            if not hasattr(w, "children") or not w.children:
                continue
            max_right = 0
            max_bottom = 0
            for child in w.children:
                max_right = max(max_right, child.x + child.width)
                max_bottom = max(max_bottom, child.y + child.height)
            if max_right > w.width:
                old_w = w.width
                w.width = max_right
                report.add_fix(
                    "container_overflow", w.name,
                    f"width {old_w} -> {w.width}",
                )
            if max_bottom > w.height:
                old_h = w.height
                w.height = max_bottom
                report.add_fix(
                    "container_overflow", w.name,
                    f"height {old_h} -> {w.height}",
                )

    def _detect_overlaps(self, widgets, report):
        for w in widgets:
            if not hasattr(w, "children") or len(w.children) < 2:
                continue
            children = w.children
            for i in range(len(children)):
                for j in range(i + 1, len(children)):
                    a, b = children[i], children[j]
                    if _rects_overlap(a.x, a.y, a.width, a.height,
                                      b.x, b.y, b.width, b.height):
                        report.add_warning(
                            "overlap", a.name,
                            f"overlaps with {b.name}",
                        )

    def _fix_image_resize(self, widgets, report):
        for w in widgets:
            if w.widget_type != "image":
                continue
            image_file = w.properties.get("image_file", "")
            if not image_file:
                continue
            resize = w.properties.get("image_resize", "")
            if not resize:
                w.properties["image_resize"] = "EGUI_IMAGE_RESIZE_SCALE"
                report.add_fix(
                    "image_crop", w.name,
                    "added image_resize=EGUI_IMAGE_RESIZE_SCALE",
                )

    def _fix_id_conflicts(self, widgets, report):
        seen = {}
        for w in widgets:
            if w.name in seen:
                old_name = w.name
                suffix = 2
                while f"{old_name}_{suffix}" in seen:
                    suffix += 1
                w.name = f"{old_name}_{suffix}"
                report.add_fix(
                    "id_conflict", w.name,
                    f"renamed from {old_name} (duplicate)",
                )
            seen[w.name] = w
