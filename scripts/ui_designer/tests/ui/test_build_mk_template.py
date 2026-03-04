"""Tests for EGUI app scaffolding content generators.

Covers build.mk template generation:
  - EGUI_CODE_INCLUDE must NOT contain '-I' prefix (Makefile adds it via patsubst)
  - EGUI_CODE_SRC must use directory paths, not individual .c files
  - No broken backslash line continuations with empty continuation
  - Makefile variable syntax is correct (+= not =)
"""

import pytest


class TestMakeAppBuildMkContent:
    """Tests for make_app_build_mk_content() in utils.scaffold."""

    @pytest.fixture
    def content(self):
        from ui_designer.utils.scaffold import make_app_build_mk_content
        return make_app_build_mk_content("MyTestApp")

    # ── presence checks ─────────────────────────────────────────

    def test_contains_app_name_comment(self, content):
        assert "MyTestApp" in content

    def test_has_egui_code_src(self, content):
        assert "EGUI_CODE_SRC" in content

    def test_has_egui_code_include(self, content):
        assert "EGUI_CODE_INCLUDE" in content

    def test_src_includes_app_path(self, content):
        assert "$(EGUI_APP_PATH)" in content

    def test_src_includes_resource(self, content):
        assert "$(EGUI_APP_PATH)/resource" in content

    def test_include_includes_app_path(self, content):
        assert "EGUI_CODE_INCLUDE\t+= $(EGUI_APP_PATH)" in content

    # ── correctness checks ───────────────────────────────────────

    def test_no_dash_i_prefix_in_include(self, content):
        """-I prefix must NOT appear — Makefile patsubst adds it automatically."""
        assert "-I$(EGUI_APP_PATH)" not in content
        assert "-I" not in content

    def test_no_individual_c_file_in_src(self, content):
        """SRC must use directories, not individual .c files like uicode.c."""
        assert "uicode.c" not in content
        assert ".c \\" not in content
        assert ".c\n" not in content.replace("$(EGUI_APP_PATH)", "")

    def test_uses_append_operator(self, content):
        """Must use += not = to avoid overwriting other modules' settings."""
        assert "+=" in content
        # No bare assignment (just = not preceded by +)
        import re
        bare_assignments = re.findall(r'(?<!\+)=\s*\$', content)
        assert bare_assignments == []

    def test_no_broken_continuation(self, content):
        """No backslash continuation with empty next line (old bug pattern)."""
        lines = content.splitlines()
        for i, line in enumerate(lines):
            if line.rstrip().endswith("\\"):
                # The next line must not be empty or whitespace-only
                if i + 1 < len(lines):
                    assert lines[i + 1].strip() != "", (
                        f"Broken continuation at line {i + 1}: '{line}' "
                        f"followed by empty line"
                    )

    def test_resource_subdirs_present(self, content):
        """resource/img and resource/font directories must be included."""
        assert "$(EGUI_APP_PATH)/resource/img" in content
        assert "$(EGUI_APP_PATH)/resource/font" in content

    def test_content_is_string(self, content):
        assert isinstance(content, str)
        assert len(content) > 0

    def test_different_app_names(self):
        """Function must accept any app name."""
        from ui_designer.utils.scaffold import make_app_build_mk_content
        for name in ("HelloSimple", "MyDashboard", "WeatherApp"):
            result = make_app_build_mk_content(name)
            assert name in result
            assert "-I" not in result


class TestMakeAppConfigHContent:
    """Tests for make_app_config_h_content() in utils.scaffold."""

    @pytest.fixture
    def content_default(self):
        from ui_designer.utils.scaffold import make_app_config_h_content
        return make_app_config_h_content("TestApp")

    def test_has_include_guard(self, content_default):
        assert "#ifndef _APP_EGUI_CONFIG_H_" in content_default
        assert "#define _APP_EGUI_CONFIG_H_" in content_default
        assert "#endif" in content_default

    def test_default_screen_width(self, content_default):
        assert "EGUI_CONFIG_SCEEN_WIDTH  240" in content_default

    def test_default_screen_height(self, content_default):
        assert "EGUI_CONFIG_SCEEN_HEIGHT 320" in content_default

    def test_custom_dimensions(self):
        from ui_designer.utils.scaffold import make_app_config_h_content
        content = make_app_config_h_content("WideApp", screen_width=480, screen_height=272)
        assert "480" in content
        assert "272" in content

    def test_pfb_dimensions_present(self, content_default):
        assert "EGUI_CONFIG_PFB_WIDTH" in content_default
        assert "EGUI_CONFIG_PFB_HEIGHT" in content_default
