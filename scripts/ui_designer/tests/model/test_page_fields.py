"""Tests for ui_designer.model.page_fields."""

from ui_designer.model.page import Page
from ui_designer.model.page_fields import (
    collect_page_field_issues,
    page_field_declaration,
    page_field_default_assignment,
    suggest_page_field_name,
    valid_page_fields,
    validate_page_fields,
)
from ui_designer.model.widget_model import AnimationModel, WidgetModel


def _make_page():
    page = Page.create_default("main_page", screen_width=240, screen_height=320)
    title = WidgetModel("label", name="title", x=12, y=16, width=120, height=24)
    alpha = AnimationModel()
    alpha.anim_type = "alpha"
    title.animations.append(alpha)
    page.root_widget.add_child(title)
    return page


class TestPageFieldValidation:
    def test_validate_page_fields_normalizes_valid_input(self):
        page = _make_page()

        ok, fields, message = validate_page_fields(
            page,
            [{"name": " counter ", "type": " int ", "default": 3}],
        )

        assert ok is True
        assert fields == [{"name": "counter", "type": "int", "default": "3"}]
        assert message == ""

    def test_collect_page_field_issues_reports_invalid_and_conflicting_names(self):
        page = _make_page()

        _, issues = collect_page_field_issues(
            page,
            [
                {"name": "bad-name", "type": "int"},
                {"name": "title", "type": "int"},
                {"name": "anim_title_alpha", "type": "int"},
            ],
        )

        assert [issue["code"] for issue in issues] == [
            "invalid_name",
            "conflict",
            "conflict",
        ]
        assert "valid C identifier" in issues[0]["message"]
        assert "auto-generated page member" in issues[1]["message"]

    def test_collect_page_field_issues_reports_empty_duplicate_and_missing_type(self):
        page = _make_page()

        _, issues = collect_page_field_issues(
            page,
            [
                {"name": "", "type": "int"},
                {"name": "counter", "type": "int"},
                {"name": "counter", "type": "uint32_t"},
                {"name": "buffer", "type": ""},
            ],
        )

        assert [issue["code"] for issue in issues] == [
            "empty_name",
            "duplicate_name",
            "duplicate_name",
            "missing_type",
        ]
        assert "Page field #1 must define a name." == issues[0]["message"]
        assert "already exists in this page" in issues[1]["message"]
        assert "must define a C type" in issues[3]["message"]

    def test_valid_page_fields_keeps_only_generator_safe_fields(self):
        page = _make_page()

        fields = valid_page_fields(
            page,
            [
                {"name": "counter", "type": "int", "default": 0},
                {"name": "title", "type": "int"},
                {"name": "counter", "type": "uint32_t"},
                {"name": "anim_title_alpha", "type": "int"},
                {"name": "buffer", "type": "uint8_t[16]"},
            ],
        )

        assert fields == [{"name": "buffer", "type": "uint8_t[16]"}]

    def test_suggest_page_field_name_avoids_generated_and_existing_names(self):
        page = _make_page()

        suggested = suggest_page_field_name(
            page,
            fields=[{"name": "field", "type": "int"}],
            base_name="title",
        )

        assert suggested == "title_2"


class TestPageFieldCodegenHelpers:
    def test_page_field_declaration_supports_scalar_pointer_and_array_types(self):
        assert page_field_declaration({"name": "count", "type": "int"}) == "int count;"
        assert page_field_declaration({"name": "text", "type": "char *"}) == "char * text;"
        assert page_field_declaration({"name": "buffer", "type": "uint8_t[32]"}) == "uint8_t buffer[32];"

    def test_page_field_default_assignment_skips_arrays(self):
        assert page_field_default_assignment({"name": "count", "type": "int", "default": 7}) == "local->count = 7;"
        assert page_field_default_assignment({"name": "text", "type": "char *", "default": "\"hi\""}) == 'local->text = "hi";'
        assert page_field_default_assignment({"name": "buffer", "type": "uint8_t[32]", "default": "{0}"}) == ""
