"""Tests for ui_designer.model.page_timers."""

from ui_designer.model.page import Page
from ui_designer.model.page_timers import (
    collect_page_timer_issues,
    normalize_page_timer,
    suggest_page_timer_callback,
    suggest_page_timer_name,
    valid_page_timers,
)
from ui_designer.model.widget_model import WidgetModel


def _make_page():
    page = Page.create_default("main_page", screen_width=240, screen_height=320)
    title = WidgetModel("label", name="title", x=10, y=10, width=100, height=20)
    page.root_widget.add_child(title)
    page.user_fields = [{"name": "counter", "type": "int", "default": "0"}]
    return page


class TestPageTimers:
    def test_normalize_page_timer_coerces_fields(self):
        timer = normalize_page_timer(
            {"name": "refresh-timer", "callback": "on refresh", "delay_ms": 250, "period_ms": 0, "auto_start": "true"}
        )

        assert timer == {
            "name": "refresh-timer",
            "callback": "on_refresh",
            "delay_ms": "250",
            "period_ms": "0",
            "auto_start": True,
        }

    def test_collect_page_timer_issues_reports_conflicts_duplicates_and_invalid_callback(self):
        page = _make_page()
        timers = [
            {"name": "title", "callback": "tick_cb", "delay_ms": "1000", "period_ms": "1000"},
            {"name": "refresh_timer", "callback": "bad-callback", "delay_ms": "1000", "period_ms": "1000"},
            {"name": "refresh_timer", "callback": "tick_ok", "delay_ms": "1000", "period_ms": "1000"},
        ]

        _, issues = collect_page_timer_issues(page, timers)

        assert [issue["code"] for issue in issues] == ["conflict", "duplicate_name", "duplicate_name"]

    def test_suggest_helpers_avoid_generated_members(self):
        page = _make_page()
        page.timers = [{"name": "timer", "callback": "egui_main_page_timer_callback", "delay_ms": "1000", "period_ms": "1000"}]

        assert suggest_page_timer_name(page, page.timers) == "timer_2"
        assert suggest_page_timer_callback(page, "status_timer") == "egui_main_page_status_timer_callback"

    def test_valid_page_timers_filters_invalid_entries(self):
        page = _make_page()
        timers = [
            {"name": "refresh_timer", "callback": "tick_cb", "delay_ms": "1000", "period_ms": "1000"},
            {"name": "title", "callback": "tick_other", "delay_ms": "1000", "period_ms": "1000"},
        ]

        assert valid_page_timers(page, timers) == [
            {
                "name": "refresh_timer",
                "callback": "tick_cb",
                "delay_ms": "1000",
                "period_ms": "1000",
                "auto_start": False,
            }
        ]
