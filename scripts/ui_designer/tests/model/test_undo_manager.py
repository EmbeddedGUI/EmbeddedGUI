"""Tests for ui_designer.model.undo_manager — PageUndoStack and UndoManager."""

import pytest

from ui_designer.model.undo_manager import PageUndoStack, UndoManager


# ── TestPageUndoStack ─────────────────────────────────────────────


class TestPageUndoStack:
    """Core push / undo / redo behaviour."""

    def test_push_then_undo_returns_previous_snapshot(self):
        stack = PageUndoStack()
        stack.push("<A/>")
        stack.push("<B/>")
        result = stack.undo()
        assert result == "<A/>"

    def test_push_then_redo_returns_next_snapshot(self):
        stack = PageUndoStack()
        stack.push("<A/>")
        stack.push("<B/>")
        stack.undo()
        result = stack.redo()
        assert result == "<B/>"

    def test_push_after_undo_truncates_redo_history(self):
        stack = PageUndoStack()
        stack.push("<A/>")
        stack.push("<B/>")
        stack.push("<C/>")
        stack.undo()  # back to B
        stack.undo()  # back to A
        stack.push("<D/>")
        assert not stack.can_redo(), "redo tail should be truncated after new push"
        assert stack.undo() == "<A/>"

    def test_can_undo_is_false_initially(self):
        stack = PageUndoStack()
        assert not stack.can_undo()

    def test_can_redo_is_false_initially(self):
        stack = PageUndoStack()
        assert not stack.can_redo()

    def test_can_undo_is_false_with_single_entry(self):
        stack = PageUndoStack()
        stack.push("<A/>")
        assert not stack.can_undo()

    def test_deduplicate_identical_consecutive_pushes(self):
        stack = PageUndoStack()
        stack.push("<A/>")
        stack.push("<A/>")
        stack.push("<A/>")
        assert not stack.can_undo(), "identical pushes should be deduplicated"

    def test_max_size_overflow_trims_oldest(self):
        stack = PageUndoStack(max_size=3)
        stack.push("<1/>")
        stack.push("<2/>")
        stack.push("<3/>")
        stack.push("<4/>")  # should evict <1/>
        # We can undo twice (4->3, 3->2) but not three times
        assert stack.can_undo()
        stack.undo()
        assert stack.can_undo()
        stack.undo()
        assert not stack.can_undo(), "oldest entry should have been evicted"

    def test_undo_at_beginning_returns_none(self):
        stack = PageUndoStack()
        stack.push("<A/>")
        assert stack.undo() is None

    def test_redo_at_end_returns_none(self):
        stack = PageUndoStack()
        stack.push("<A/>")
        assert stack.redo() is None

    def test_clear_resets_everything(self):
        stack = PageUndoStack()
        stack.push("<A/>")
        stack.push("<B/>")
        stack.clear()
        assert not stack.can_undo()
        assert not stack.can_redo()


# ── TestDirtyTracking ────────────────────────────────────────────


class TestDirtyTracking:
    """Save / dirty flag behaviour."""

    def test_not_dirty_initially(self):
        stack = PageUndoStack()
        assert not stack.is_dirty(), "empty stack should not be dirty"

    def test_push_makes_dirty(self):
        stack = PageUndoStack()
        stack.push("<A/>")
        assert stack.is_dirty()

    def test_mark_saved_clears_dirty(self):
        stack = PageUndoStack()
        stack.push("<A/>")
        stack.mark_saved()
        assert not stack.is_dirty()

    def test_undo_past_save_is_dirty(self):
        stack = PageUndoStack()
        stack.push("<A/>")
        stack.push("<B/>")
        stack.mark_saved()  # saved at B
        stack.undo()  # back to A
        assert stack.is_dirty(), "undoing past save point should be dirty"

    def test_redo_back_to_save_clears_dirty(self):
        stack = PageUndoStack()
        stack.push("<A/>")
        stack.push("<B/>")
        stack.mark_saved()  # saved at B
        stack.undo()
        assert stack.is_dirty()
        stack.redo()
        assert not stack.is_dirty()

    def test_overflow_discards_save_point_permanently_dirty(self):
        stack = PageUndoStack(max_size=3)
        stack.push("<1/>")
        stack.mark_saved()  # save_index = 0
        stack.push("<2/>")
        stack.push("<3/>")
        stack.push("<4/>")  # evicts <1/>, save_index goes negative -> -2
        assert stack.is_dirty(), "save point evicted, should be permanently dirty"


# ── TestBatchOperations ──────────────────────────────────────────


class TestBatchOperations:
    """Batch coalescing (drag / resize)."""

    def test_push_suppressed_during_batch(self):
        stack = PageUndoStack()
        stack.push("<A/>")
        stack.begin_batch()
        stack.push("<B/>")
        stack.push("<C/>")
        # B and C should not have been recorded
        assert not stack.can_redo()
        # Still at A after cancelling batch via end_batch with same value
        stack.end_batch("<A/>")
        assert not stack.can_undo() or stack.undo() == "<A/>"

    def test_end_batch_commits_final_state(self):
        stack = PageUndoStack()
        stack.push("<A/>")
        stack.begin_batch()
        stack.push("<ignored/>")
        stack.end_batch("<Z/>")
        assert stack.can_undo()
        assert stack.undo() == "<A/>"

    def test_no_net_change_discards_batch(self):
        stack = PageUndoStack()
        stack.push("<A/>")
        stack.begin_batch()
        stack.end_batch("<A/>")  # same as before batch
        assert not stack.can_undo(), "batch with no net change should not add entry"

    def test_nested_begin_batch_ignored(self):
        stack = PageUndoStack()
        stack.push("<A/>")
        stack.begin_batch()
        stack.begin_batch()  # second begin_batch should be ignored
        stack.end_batch("<B/>")
        assert stack.can_undo()
        assert stack.undo() == "<A/>"

    def test_end_batch_without_begin_does_nothing(self):
        stack = PageUndoStack()
        stack.push("<A/>")
        stack.end_batch("<B/>")  # no active batch, should be no-op
        assert not stack.can_undo(), "end_batch without begin should be no-op"


# ── TestUndoManager ──────────────────────────────────────────────


class TestUndoManager:
    """Multi-page undo manager."""

    def test_per_page_stacks_are_independent(self):
        mgr = UndoManager()
        s1 = mgr.get_stack("page1")
        s2 = mgr.get_stack("page2")
        s1.push("<A/>")
        assert s1.can_undo() is False  # only one entry
        assert s2.can_undo() is False
        s1.push("<B/>")
        assert s1.can_undo() is True
        assert s2.can_undo() is False

    def test_get_stack_creates_on_demand(self):
        mgr = UndoManager()
        s = mgr.get_stack("new_page")
        assert isinstance(s, PageUndoStack)
        # same instance returned again
        assert mgr.get_stack("new_page") is s

    def test_remove_stack(self):
        mgr = UndoManager()
        mgr.get_stack("p1")
        mgr.remove_stack("p1")
        # accessing again should create a fresh stack
        s = mgr.get_stack("p1")
        assert not s.can_undo()

    def test_rename_stack_preserves_history(self):
        mgr = UndoManager()
        s = mgr.get_stack("old")
        s.push("<A/>")
        s.push("<B/>")
        mgr.rename_stack("old", "new")
        # old name gone
        fresh = mgr.get_stack("old")
        assert not fresh.can_undo()
        # new name has history
        renamed = mgr.get_stack("new")
        assert renamed.can_undo()
        assert renamed.undo() == "<A/>"

    def test_is_any_dirty(self):
        mgr = UndoManager()
        mgr.get_stack("p1").push("<A/>")
        assert mgr.is_any_dirty()

    def test_is_any_dirty_false_when_all_clean(self):
        mgr = UndoManager()
        mgr.get_stack("p1")
        mgr.get_stack("p2")
        assert not mgr.is_any_dirty()

    def test_mark_all_saved(self):
        mgr = UndoManager()
        mgr.get_stack("p1").push("<A/>")
        mgr.get_stack("p2").push("<B/>")
        assert mgr.is_any_dirty()
        mgr.mark_all_saved()
        assert not mgr.is_any_dirty()

    def test_clear_removes_all_stacks(self):
        mgr = UndoManager()
        mgr.get_stack("p1").push("<A/>")
        mgr.get_stack("p2").push("<B/>")
        mgr.clear()
        assert not mgr.is_any_dirty()
        # Accessing after clear gives fresh stacks
        assert not mgr.get_stack("p1").can_undo()
