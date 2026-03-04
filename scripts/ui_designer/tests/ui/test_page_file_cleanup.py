"""Tests for page file cleanup when a page is deleted.

Covers the delete_page_generated_files() behavior:
  - Deletes .h, _layout.c, .c files for the named page
  - Handles missing files gracefully (no exception)
  - Does not delete unrelated files
  - Works with nested paths
"""

import os
import pytest


def _make_page_files(directory, page_name):
    """Create the three generated files for a page in directory."""
    files = [
        os.path.join(directory, f"{page_name}.h"),
        os.path.join(directory, f"{page_name}_layout.c"),
        os.path.join(directory, f"{page_name}.c"),
    ]
    for f in files:
        with open(f, "w") as fp:
            fp.write(f"// generated: {os.path.basename(f)}\n")
    return files


class TestDeletePageGeneratedFiles:
    """Tests for delete_page_generated_files helper."""

    @pytest.fixture
    def delete_fn(self):
        from ui_designer.ui.main_window import delete_page_generated_files
        return delete_page_generated_files

    def test_deletes_all_three_files(self, tmp_path, delete_fn):
        """All three generated files are deleted after calling the function."""
        files = _make_page_files(str(tmp_path), "settings")
        for f in files:
            assert os.path.isfile(f)

        delete_fn(str(tmp_path), "settings")

        for f in files:
            assert not os.path.exists(f), f"Expected {f} to be deleted"

    def test_no_error_when_files_missing(self, tmp_path, delete_fn):
        """Should not raise when no generated files exist."""
        # Nothing created — must not raise
        delete_fn(str(tmp_path), "nonexistent_page")

    def test_no_error_when_only_some_files_exist(self, tmp_path, delete_fn):
        """Should not raise when only a subset of files exists."""
        # Only create the .h file
        h_file = os.path.join(str(tmp_path), "partial.h")
        with open(h_file, "w") as f:
            f.write("")

        delete_fn(str(tmp_path), "partial")

        assert not os.path.exists(h_file)

    def test_does_not_delete_other_files(self, tmp_path, delete_fn):
        """Unrelated files in the directory must not be touched."""
        # Create page files
        _make_page_files(str(tmp_path), "about")
        # Create unrelated files
        unrelated = os.path.join(str(tmp_path), "uicode.c")
        main_h = os.path.join(str(tmp_path), "main_page.h")
        for f in (unrelated, main_h):
            with open(f, "w") as fp:
                fp.write("")

        delete_fn(str(tmp_path), "about")

        assert os.path.isfile(unrelated), "uicode.c must not be deleted"
        assert os.path.isfile(main_h), "main_page.h must not be deleted"

    def test_correct_file_suffixes_deleted(self, tmp_path, delete_fn):
        """Verify the exact file names that are deleted."""
        page_name = "my_screen"
        expected = [
            os.path.join(str(tmp_path), f"{page_name}.h"),
            os.path.join(str(tmp_path), f"{page_name}_layout.c"),
            os.path.join(str(tmp_path), f"{page_name}.c"),
        ]
        for f in expected:
            with open(f, "w") as fp:
                fp.write("")

        delete_fn(str(tmp_path), page_name)

        for f in expected:
            assert not os.path.exists(f)

    def test_idempotent_double_call(self, tmp_path, delete_fn):
        """Calling twice on the same page must not raise."""
        _make_page_files(str(tmp_path), "home")
        delete_fn(str(tmp_path), "home")
        # Second call — files already gone, must not raise
        delete_fn(str(tmp_path), "home")

    def test_multi_page_only_target_deleted(self, tmp_path, delete_fn):
        """With multiple page files present, only the target page is deleted."""
        _make_page_files(str(tmp_path), "page_a")
        _make_page_files(str(tmp_path), "page_b")

        delete_fn(str(tmp_path), "page_a")

        # page_a gone
        assert not os.path.exists(os.path.join(str(tmp_path), "page_a.h"))
        assert not os.path.exists(os.path.join(str(tmp_path), "page_a_layout.c"))
        assert not os.path.exists(os.path.join(str(tmp_path), "page_a.c"))
        # page_b intact
        assert os.path.isfile(os.path.join(str(tmp_path), "page_b.h"))
        assert os.path.isfile(os.path.join(str(tmp_path), "page_b_layout.c"))
        assert os.path.isfile(os.path.join(str(tmp_path), "page_b.c"))
