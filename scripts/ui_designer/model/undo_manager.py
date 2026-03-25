"""Undo/Redo manager for EmbeddedGUI Designer.

Uses per-page XML snapshot stacks. Each state-changing operation captures
the full page XML string, making undo/redo a simple snapshot restore.
"""


class PageUndoStack:
    """Undo/redo stack for a single page, backed by XML string snapshots.

    The stack maintains a linear history list and a current-position index.
    Undo moves the index backward; redo moves it forward.  A new push
    truncates any redo history beyond the current index.

    Dirty tracking compares the current index against the index recorded
    at the last save, so undoing past the save point correctly reports
    the page as dirty.
    """

    def __init__(self, max_size=50):
        self._history = []      # list of XML snapshot strings
        self._labels = []       # user-facing labels for each snapshot
        self._index = -1        # points to current state in _history
        self._save_index = -1   # index at last save
        self._max_size = max_size
        self._batch_active = False
        self._batch_start_index = -1  # index before batch started

    # ── Push / Undo / Redo ────────────────────────────────────────

    def push(self, xml_snapshot, label="State capture"):
        """Record a new snapshot.

        If a batch is active, intermediate pushes are suppressed — only
        the state when end_batch() is called will be committed.
        """
        if self._batch_active:
            # During a batch, keep replacing the "pending" top without
            # growing the stack.  The real entry is committed in end_batch().
            return

        # Deduplicate: skip if identical to current state
        if self._index >= 0 and self._history[self._index] == xml_snapshot:
            return

        # Truncate any redo tail
        del self._history[self._index + 1:]
        del self._labels[self._index + 1:]

        self._history.append(xml_snapshot)
        self._labels.append(label or "State capture")
        self._index = len(self._history) - 1

        # Enforce max size
        if len(self._history) > self._max_size:
            overflow = len(self._history) - self._max_size
            del self._history[:overflow]
            del self._labels[:overflow]
            self._index -= overflow
            if self._save_index >= 0:
                self._save_index -= overflow
                if self._save_index < 0:
                    # Save point has been discarded — page is permanently dirty
                    self._save_index = -2

    def undo(self):
        """Move one step back. Returns the XML snapshot to restore, or None."""
        if not self.can_undo():
            return None
        self._index -= 1
        return self._history[self._index]

    def redo(self):
        """Move one step forward. Returns the XML snapshot to restore, or None."""
        if not self.can_redo():
            return None
        self._index += 1
        return self._history[self._index]

    def can_undo(self):
        return self._index > 0

    def can_redo(self):
        return self._index < len(self._history) - 1

    # ── Dirty / Save tracking ─────────────────────────────────────

    def mark_saved(self):
        """Record the current position as the "saved" state."""
        self._save_index = self._index

    def is_dirty(self):
        """True if current state differs from last-saved state."""
        return self._index != self._save_index

    # ── Batch operations (drag / resize coalescing) ───────────────

    def begin_batch(self):
        """Start a batch: many intermediate mutations will be coalesced
        into a single undo entry when end_batch() is called."""
        if self._batch_active:
            return
        self._batch_active = True
        self._batch_start_index = self._index

    def end_batch(self, xml_snapshot, label="Batch edit"):
        """End a batch and commit the final state as one undo entry.

        If the final state is identical to the state before the batch,
        nothing is pushed.
        """
        if not self._batch_active:
            return
        self._batch_active = False

        # Compare with state before batch
        if (self._batch_start_index >= 0
                and self._history[self._batch_start_index] == xml_snapshot):
            # No net change — discard
            return

        # Commit the final state normally
        # Truncate any redo tail
        del self._history[self._index + 1:]
        del self._labels[self._index + 1:]
        self._history.append(xml_snapshot)
        self._labels.append(label or "Batch edit")
        self._index = len(self._history) - 1

        # Enforce max size
        if len(self._history) > self._max_size:
            overflow = len(self._history) - self._max_size
            del self._history[:overflow]
            del self._labels[:overflow]
            self._index -= overflow
            if self._save_index >= 0:
                self._save_index -= overflow
                if self._save_index < 0:
                    self._save_index = -2

    def current_label(self):
        """Return the label for the current snapshot."""
        if self._index < 0 or self._index >= len(self._labels):
            return ""
        return self._labels[self._index]

    def history_entries(self):
        """Return structured entries for history visualization."""
        entries = []
        for index, (xml_snapshot, label) in enumerate(zip(self._history, self._labels)):
            entries.append({
                "index": index,
                "xml": xml_snapshot,
                "label": label,
                "is_current": index == self._index,
                "is_saved": index == self._save_index,
            })
        return entries

    def clear(self):
        """Discard all history."""
        self._history.clear()
        self._labels.clear()
        self._index = -1
        self._save_index = -1
        self._batch_active = False
        self._batch_start_index = -1


class UndoManager:
    """Manages per-page undo stacks for the entire project."""

    def __init__(self):
        self._stacks = {}  # page_name -> PageUndoStack

    def get_stack(self, page_name):
        """Get or create the undo stack for a page."""
        if page_name not in self._stacks:
            self._stacks[page_name] = PageUndoStack()
        return self._stacks[page_name]

    def remove_stack(self, page_name):
        """Remove the undo stack when a page is deleted."""
        self._stacks.pop(page_name, None)

    def rename_stack(self, old_name, new_name):
        """Migrate the undo stack when a page is renamed."""
        stack = self._stacks.pop(old_name, None)
        if stack is not None:
            self._stacks[new_name] = stack

    def is_any_dirty(self):
        """True if any page has unsaved changes."""
        return any(s.is_dirty() for s in self._stacks.values())

    def dirty_pages(self):
        """Return page names that currently differ from the last saved state."""
        return [name for name, stack in self._stacks.items() if stack.is_dirty()]

    def mark_all_saved(self):
        """Mark all pages as saved."""
        for stack in self._stacks.values():
            stack.mark_saved()

    def current_label(self, page_name):
        """Return the current snapshot label for the given page."""
        stack = self._stacks.get(page_name)
        if stack is None:
            return ""
        return stack.current_label()

    def history_entries(self, page_name):
        """Return structured history entries for the given page."""
        stack = self._stacks.get(page_name)
        if stack is None:
            return []
        return stack.history_entries()

    def clear(self):
        """Clear all undo stacks."""
        self._stacks.clear()
