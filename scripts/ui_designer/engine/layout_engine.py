"""Python layout engine - replicates C-side LinearLayout positioning algorithm.

Computes display_x/display_y for each widget so the overlay matches the
actual exe rendering. Based on egui_view_group.c and egui_common.c.
"""

from ..model.widget_registry import WidgetRegistry

# Alignment constants (must match egui_common.h:169-179)
EGUI_ALIGN_HCENTER = 0x01
EGUI_ALIGN_LEFT    = 0x02
EGUI_ALIGN_RIGHT   = 0x04
EGUI_ALIGN_HMASK   = 0x0F

EGUI_ALIGN_VCENTER = 0x10
EGUI_ALIGN_TOP     = 0x20
EGUI_ALIGN_BOTTOM  = 0x40
EGUI_ALIGN_VMASK   = 0xF0

# String to numeric mapping
ALIGN_MAP = {
    "EGUI_ALIGN_CENTER":  EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER,  # 0x11
    "EGUI_ALIGN_LEFT":    EGUI_ALIGN_LEFT,
    "EGUI_ALIGN_RIGHT":   EGUI_ALIGN_RIGHT,
    "EGUI_ALIGN_TOP":     EGUI_ALIGN_TOP,
    "EGUI_ALIGN_BOTTOM":  EGUI_ALIGN_BOTTOM,
    "EGUI_ALIGN_HCENTER": EGUI_ALIGN_HCENTER,
    "EGUI_ALIGN_VCENTER": EGUI_ALIGN_VCENTER,
}


def align_get_x_y(parent_w, parent_h, child_w, child_h, align_type):
    """Compute alignment offset. Replicates egui_common_align_get_x_y."""
    x = 0
    y = 0

    h_align = align_type & EGUI_ALIGN_HMASK
    if h_align == EGUI_ALIGN_HCENTER:
        if parent_w > child_w:
            x = (parent_w - child_w) >> 1
    elif h_align == EGUI_ALIGN_LEFT:
        x = 0
    elif h_align == EGUI_ALIGN_RIGHT:
        if parent_w > child_w:
            x = parent_w - child_w

    v_align = align_type & EGUI_ALIGN_VMASK
    if v_align == EGUI_ALIGN_VCENTER:
        if parent_h > child_h:
            y = (parent_h - child_h) >> 1
    elif v_align == EGUI_ALIGN_TOP:
        y = 0
    elif v_align == EGUI_ALIGN_BOTTOM:
        if parent_h > child_h:
            y = parent_h - child_h

    return x, y


def _has_layout_func(widget):
    """Check if a widget type has a layout function."""
    type_info = WidgetRegistry.instance().get(widget.widget_type)
    return type_info.get("layout_func") is not None


def _layout_linearlayout_children(widget):
    """Compute child positions for a LinearLayout container.

    Replicates egui_view_group_layout_childs from egui_view_group.c:151-221.
    Sets display_x/display_y on each child RELATIVE to the parent.
    """
    children = widget.children
    if not children:
        return

    is_horizontal = widget.properties.get("orientation", "vertical") == "horizontal"
    align_str = widget.properties.get("align_type", "EGUI_ALIGN_CENTER")
    align_type = ALIGN_MAP.get(align_str, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER)

    # Step 1: Calculate total child dimensions
    # Note: current designer doesn't support margins, so margin = 0
    if is_horizontal:
        total_child_width = sum(c.width for c in children)
        total_child_height = max(c.height for c in children) if children else 0
    else:
        total_child_width = max(c.width for c in children) if children else 0
        total_child_height = sum(c.height for c in children)

    parent_w = widget.width
    parent_h = widget.height

    # Step 2: Calculate base position
    base_x, base_y = align_get_x_y(
        parent_w, parent_h,
        total_child_width, total_child_height,
        align_type
    )

    # Step 3: Position each child
    x, y = base_x, base_y
    for child in children:
        if is_horizontal:
            # Vertical alignment for each child within total height
            child_x, child_y = align_get_x_y(
                total_child_width, total_child_height,
                child.width, child.height,
                align_type & EGUI_ALIGN_VMASK
            )
            child.display_x = x + child_x
            child.display_y = y + child_y
            x += child.width
        else:
            # Horizontal alignment for each child within total width
            child_x, child_y = align_get_x_y(
                total_child_width, total_child_height,
                child.width, child.height,
                align_type & EGUI_ALIGN_HMASK
            )
            child.display_x = x + child_x
            child.display_y = y + child_y
            y += child.height


def _compute_widget_layout(widget, parent_display_x=0, parent_display_y=0):
    """Recursively compute display coordinates for a widget and its children.

    For root widgets: display_x/y = model x/y
    For LinearLayout children: display_x/y computed by layout algorithm
    For Group children: display_x/y = model x/y (relative to parent)
    All converted to absolute screen coordinates.
    """
    # This widget's absolute screen position is already set by the caller
    # (either from model x,y for roots, or from layout algorithm for children)
    abs_x = widget.display_x
    abs_y = widget.display_y

    if not widget.children:
        return

    if _has_layout_func(widget):
        # LinearLayout / Scroll / ViewPage: compute child positions via algorithm
        _layout_linearlayout_children(widget)
        # Convert child positions from relative-to-parent to absolute screen coords
        for child in widget.children:
            child.display_x += abs_x
            child.display_y += abs_y
    else:
        # Group or other container: children use their model x,y relative to parent
        for child in widget.children:
            child.display_x = child.x + abs_x
            child.display_y = child.y + abs_y

    # Recurse into children
    for child in widget.children:
        _compute_widget_layout(child)


def compute_layout(project):
    """Compute display_x/display_y for all widgets in the project.

    Must be called after any model change and before updating the overlay.
    """
    if not project:
        return

    for root in project.root_widgets:
        # Root widgets use their model x,y as absolute position
        root.display_x = root.x
        root.display_y = root.y
        _compute_widget_layout(root)


def compute_page_layout(page):
    """Compute display_x/display_y for all widgets in a single page.

    Use this instead of compute_layout() when working with individual pages.
    """
    if not page or not page.root_widget:
        return

    root = page.root_widget
    root.display_x = root.x
    root.display_y = root.y
    _compute_widget_layout(root)
