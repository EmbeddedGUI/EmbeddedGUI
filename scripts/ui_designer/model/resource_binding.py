"""Helpers for binding project resources to widget properties."""

from .widget_registry import WidgetRegistry


RESOURCE_PROPERTY_TYPES = {
    "image": "image_file",
    "font": "font_file",
    "text": "text_file",
}


def resource_property_type(resource_type):
    """Return the widget property type that matches a resource type."""
    return RESOURCE_PROPERTY_TYPES.get(resource_type, "")


def find_widget_resource_property(widget, resource_type):
    """Find the best widget property name for a resource type.

    Prefers a property whose name exactly matches the property type
    (`image_file`, `font_file`, `text_file`) and otherwise falls back to the
    first descriptor property with the matching type. This allows widgets like
    ``font_text_file`` to participate in text resource assignment.
    """
    if widget is None:
        return ""

    prop_type = resource_property_type(resource_type)
    if not prop_type:
        return ""

    descriptor = WidgetRegistry.instance().get(widget.widget_type)
    properties = descriptor.get("properties", {})

    exact_prop = prop_type
    exact_info = properties.get(exact_prop)
    if exact_info and exact_info.get("type") == prop_type and exact_prop in widget.properties:
        return exact_prop

    for prop_name, prop_info in properties.items():
        if prop_info.get("type") != prop_type:
            continue
        if prop_name in widget.properties:
            return prop_name

    return ""


def assign_resource_to_widget(widget, resource_type, filename):
    """Assign a resource filename to the matching widget property."""
    prop_name = find_widget_resource_property(widget, resource_type)
    if not prop_name:
        return ""

    widget.properties[prop_name] = filename
    return prop_name
