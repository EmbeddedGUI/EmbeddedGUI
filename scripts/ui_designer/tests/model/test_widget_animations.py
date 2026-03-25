"""Tests for ui_designer.model.widget_animations."""

from ui_designer.model.widget_animations import (
    animation_param_choices,
    animation_param_defaults,
    clone_animation,
    create_default_animation,
    normalize_animation,
    normalize_widget_animations,
)


class TestWidgetAnimationsHelpers:
    def test_create_default_animation_uses_type_specific_defaults(self):
        animation = create_default_animation("resize")

        assert animation.anim_type == "resize"
        assert animation.duration == 500
        assert animation.repeat_mode == "restart"
        assert animation.params["from_width_ratio"] == "EGUI_FLOAT_VALUE(1.0f)"
        assert animation.params["mode"] == "EGUI_ANIMATION_RESIZE_MODE_CENTER"

    def test_normalize_animation_recovers_invalid_values(self):
        animation = normalize_animation(
            {
                "anim_type": "unknown",
                "duration": "-30",
                "interpolator": "missing",
                "repeat_count": "-1",
                "repeat_mode": "bad",
                "auto_start": "false",
                "params": {"from_alpha": "", "to_alpha": "EGUI_ALPHA_80"},
            }
        )

        assert animation.anim_type == "alpha"
        assert animation.duration == 0
        assert animation.interpolator == "linear"
        assert animation.repeat_count == 0
        assert animation.repeat_mode == "restart"
        assert animation.auto_start is False
        assert animation.params["from_alpha"] == "EGUI_ALPHA_100"
        assert animation.params["to_alpha"] == "EGUI_ALPHA_80"

    def test_clone_and_normalize_widget_animations_return_animation_models(self):
        source = create_default_animation("translate")
        source.duration = 900
        source.params["to_y"] = "48"

        clone = clone_animation(source)
        normalized = normalize_widget_animations([source, {"anim_type": "color"}])

        assert clone is not source
        assert clone.to_dict() == source.to_dict()
        assert len(normalized) == 2
        assert normalized[0].anim_type == "translate"
        assert normalized[1].anim_type == "color"

    def test_param_defaults_and_choices_cover_supported_presets(self):
        assert animation_param_defaults("alpha") == {
            "from_alpha": "EGUI_ALPHA_100",
            "to_alpha": "EGUI_ALPHA_20",
        }
        assert "EGUI_ALPHA_100" in animation_param_choices("from_alpha")
        assert "EGUI_COLOR_ORANGE" in animation_param_choices("to_color")
        assert "EGUI_ANIMATION_RESIZE_MODE_CENTER" in animation_param_choices("mode")
