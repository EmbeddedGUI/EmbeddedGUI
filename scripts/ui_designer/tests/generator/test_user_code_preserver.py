"""Tests for ui_designer.generator.user_code_preserver module."""

import os
import pytest

from ui_designer.generator.user_code_preserver import (
    extract_user_code,
    inject_user_code,
    compute_source_hash,
    embed_source_hash,
    should_skip_generation,
    get_embedded_hash,
)


# ======================================================================
# TestExtractUserCode
# ======================================================================


class TestExtractUserCode:
    """Tests for extract_user_code."""

    def test_extract_single_block(self):
        content = (
            "// some preamble\n"
            "// USER CODE BEGIN callbacks\n"
            "static void my_cb(void) {}\n"
            "// USER CODE END callbacks\n"
            "// more code\n"
        )
        blocks = extract_user_code(content)
        assert "callbacks" in blocks
        assert "static void my_cb(void) {}" in blocks["callbacks"]

    def test_extract_multiple_blocks(self):
        content = (
            "// USER CODE BEGIN includes\n"
            '#include "foo.h"\n'
            "// USER CODE END includes\n"
            "\n"
            "// USER CODE BEGIN user_fields\n"
            "int counter;\n"
            "// USER CODE END user_fields\n"
            "\n"
            "// USER CODE BEGIN declarations\n"
            "void do_stuff(void);\n"
            "// USER CODE END declarations\n"
        )
        blocks = extract_user_code(content)
        assert len(blocks) == 3
        assert "includes" in blocks
        assert "user_fields" in blocks
        assert "declarations" in blocks

    def test_extract_empty_block(self):
        content = (
            "// USER CODE BEGIN empty\n"
            "// USER CODE END empty\n"
        )
        blocks = extract_user_code(content)
        assert "empty" in blocks
        assert blocks["empty"] == ""

    def test_extract_no_markers(self):
        content = "int main() { return 0; }\n"
        blocks = extract_user_code(content)
        assert blocks == {}

    def test_extract_preserves_indentation(self):
        content = (
            "    // USER CODE BEGIN init\n"
            "    my_init();\n"
            "    setup_timer(100);\n"
            "    // USER CODE END init\n"
        )
        blocks = extract_user_code(content)
        assert "init" in blocks
        assert "    my_init();\n" in blocks["init"]
        assert "    setup_timer(100);\n" in blocks["init"]

    def test_extract_multiline_body(self):
        content = (
            "// USER CODE BEGIN body\n"
            "line1\n"
            "line2\n"
            "line3\n"
            "// USER CODE END body\n"
        )
        blocks = extract_user_code(content)
        assert "body" in blocks
        body = blocks["body"]
        assert "line1" in body
        assert "line2" in body
        assert "line3" in body


# ======================================================================
# TestInjectUserCode
# ======================================================================


class TestInjectUserCode:
    """Tests for inject_user_code."""

    def test_inject_restores_code(self):
        old_content = (
            "// USER CODE BEGIN includes\n"
            '#include "my_lib.h"\n'
            "// USER CODE END includes\n"
        )
        blocks = extract_user_code(old_content)

        new_template = (
            "// USER CODE BEGIN includes\n"
            "// USER CODE END includes\n"
        )
        result = inject_user_code(new_template, blocks)
        assert '#include "my_lib.h"' in result

    def test_inject_empty_blocks(self):
        content = (
            "// USER CODE BEGIN foo\n"
            "// USER CODE END foo\n"
        )
        result = inject_user_code(content, {})
        assert "// USER CODE BEGIN foo" in result
        assert "// USER CODE END foo" in result

    def test_inject_missing_tag(self):
        """Blocks has a tag that does not exist in the generated content."""
        generated = "int x = 0;\n"
        blocks = {"orphan_tag": "// orphaned code\n"}
        result = inject_user_code(generated, blocks)
        # The orphaned code should be appended
        assert "orphan_tag" in result
        assert "orphaned code" in result

    def test_inject_orphaned_code_appended(self):
        generated = (
            "// USER CODE BEGIN includes\n"
            "// USER CODE END includes\n"
        )
        blocks = {
            "includes": '#include "ok.h"\n',
            "removed_section": "void old_func(void);\n",
        }
        result = inject_user_code(generated, blocks)
        assert '#include "ok.h"' in result
        assert "removed_section" in result
        assert "old_func" in result

    def test_inject_only_non_empty_orphans(self):
        """Empty orphan body should NOT be appended."""
        generated = "int x = 0;\n"
        blocks = {"empty_orphan": ""}
        result = inject_user_code(generated, blocks)
        # empty orphans are filtered out (.strip() is empty)
        assert "empty_orphan" not in result


# ======================================================================
# TestComputeSourceHash
# ======================================================================


class TestComputeSourceHash:
    """Tests for compute_source_hash."""

    def test_hash_deterministic(self):
        text = "void foo(void) {}"
        h1 = compute_source_hash(text)
        h2 = compute_source_hash(text)
        assert h1 == h2

    def test_hash_changes_with_content(self):
        h1 = compute_source_hash("version_a")
        h2 = compute_source_hash("version_b")
        assert h1 != h2

    def test_hash_length(self):
        h = compute_source_hash("some content")
        assert len(h) == 16


# ======================================================================
# TestEmbedSourceHash
# ======================================================================


class TestEmbedSourceHash:
    """Tests for embed_source_hash."""

    def test_embed_prepends_hash(self):
        content = '#include "egui.h"\nvoid foo() {}\n'
        result = embed_source_hash(content, "abcd1234abcd1234")
        assert result.startswith("// GENERATED_HASH: abcd1234abcd1234\n")
        assert '#include "egui.h"' in result

    def test_embed_replaces_existing(self):
        content = (
            "// GENERATED_HASH: aabbccdd11223344\n"
            '#include "egui.h"\n'
        )
        result = embed_source_hash(content, "5566778899aabbcc")
        assert "// GENERATED_HASH: 5566778899aabbcc" in result
        assert "aabbccdd11223344" not in result
        # Should not have duplicate hash lines
        assert result.count("GENERATED_HASH") == 1


# ======================================================================
# TestShouldSkipGeneration
# ======================================================================


class TestShouldSkipGeneration:
    """Tests for should_skip_generation (uses filesystem)."""

    def test_skip_when_hash_matches(self, tmp_path):
        content = "// some generated code\n"
        source_hash = compute_source_hash(content)
        file_content = embed_source_hash(content, source_hash)

        filepath = tmp_path / "test.c"
        filepath.write_text(file_content, encoding="utf-8")

        assert should_skip_generation(str(filepath), source_hash) is True

    def test_no_skip_when_hash_differs(self, tmp_path):
        content = "// old generated code\n"
        old_hash = compute_source_hash(content)
        file_content = embed_source_hash(content, old_hash)

        filepath = tmp_path / "test.c"
        filepath.write_text(file_content, encoding="utf-8")

        new_hash = compute_source_hash("// updated code\n")
        assert should_skip_generation(str(filepath), new_hash) is False

    def test_no_skip_when_file_missing(self, tmp_path):
        filepath = tmp_path / "nonexistent.c"
        assert should_skip_generation(str(filepath), "any_hash_value_x") is False


# ======================================================================
# TestRoundTrip
# ======================================================================


class TestRoundTrip:
    """Round-trip: extract from existing file, inject into fresh template."""

    def test_extract_then_inject_round_trip(self, test_data_dir):
        sample_path = os.path.join(test_data_dir, "user_code_sample.h")
        with open(sample_path, "r", encoding="utf-8") as f:
            original = f.read()

        blocks = extract_user_code(original)
        assert len(blocks) >= 3  # includes, user_fields, declarations

        # Generate a fresh template (empty USER CODE regions)
        fresh_template = (
            "#ifndef _MAIN_PAGE_H_\n"
            "#define _MAIN_PAGE_H_\n"
            "\n"
            '#include "egui.h"\n'
            "\n"
            "// USER CODE BEGIN includes\n"
            "// USER CODE END includes\n"
            "\n"
            "struct egui_main_page {\n"
            "    egui_page_base_t base;\n"
            "    // USER CODE BEGIN user_fields\n"
            "    // USER CODE END user_fields\n"
            "};\n"
            "\n"
            "// USER CODE BEGIN declarations\n"
            "// USER CODE END declarations\n"
            "\n"
            "#endif\n"
        )

        result = inject_user_code(fresh_template, blocks)

        # Verify all user code was preserved
        assert '#include "my_custom.h"' in result
        assert '#include "sensors.h"' in result
        assert "int my_counter;" in result
        assert "float my_timer;" in result
        assert "void my_custom_func(void);" in result
        assert "int get_sensor_value(int channel);" in result
