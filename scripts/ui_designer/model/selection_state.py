"""Selection state helpers for EmbeddedGUI Designer."""


class SelectionState:
    """Ordered widget selection with a primary widget.

    The selection keeps insertion order so UI surfaces can preserve the
    current primary widget while still operating on a stable widget list.
    """

    def __init__(self):
        self._widgets = []
        self._primary = None

    @property
    def widgets(self):
        return list(self._widgets)

    @property
    def primary(self):
        return self._primary

    def clear(self):
        self._widgets = []
        self._primary = None

    def set(self, widget):
        if widget is None:
            self.clear()
            return
        self._widgets = [widget]
        self._primary = widget

    def set_widgets(self, widgets, primary=None):
        ordered = []
        seen = set()
        for widget in widgets or []:
            if widget is None:
                continue
            ident = id(widget)
            if ident in seen:
                continue
            ordered.append(widget)
            seen.add(ident)

        if not ordered:
            self.clear()
            return

        if primary is None or all(widget is not primary for widget in ordered):
            primary = ordered[-1]

        self._widgets = ordered
        self._primary = primary

    def toggle(self, widget):
        if widget is None:
            return
        if any(existing is widget for existing in self._widgets):
            self.remove(widget)
            return
        self._widgets.append(widget)
        self._primary = widget

    def remove(self, widget):
        self._widgets = [existing for existing in self._widgets if existing is not widget]
        if self._primary is widget:
            self._primary = self._widgets[-1] if self._widgets else None

    def contains(self, widget):
        return any(existing is widget for existing in self._widgets)

    def is_empty(self):
        return not self._widgets

    def is_multi(self):
        return len(self._widgets) > 1
