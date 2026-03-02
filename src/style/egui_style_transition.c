#include "egui_style_transition.h"

egui_color_t egui_color_lerp(egui_color_t from, egui_color_t to, uint8_t fraction)
{
    /* Use existing egui_rgb_mix: fore_alpha=0 means all back(from), 255 means all fore(to) */
    return egui_rgb_mix(from, to, fraction);
}

egui_alpha_t egui_alpha_lerp(egui_alpha_t from, egui_alpha_t to, uint8_t fraction)
{
    return (egui_alpha_t)((uint16_t)from + (((int16_t)to - (int16_t)from) * fraction / 255));
}

egui_dim_t egui_dim_lerp(egui_dim_t from, egui_dim_t to, uint8_t fraction)
{
    return (egui_dim_t)(from + ((int32_t)(to - from) * fraction / 255));
}

void egui_style_transition_init(egui_style_transition_t *trans, const egui_style_t *from, const egui_style_t *to, uint16_t prop_mask)
{
    if (from == NULL || to == NULL)
    {
        trans->is_active = 0;
        return;
    }
    trans->from = from;
    trans->to = to;
    trans->prop_mask = prop_mask;
    trans->is_active = 1;
    /* Start with 'from' values */
    trans->current = *from;
    trans->current.flags = from->flags | to->flags;
}

void egui_style_transition_update(egui_style_transition_t *trans, uint8_t fraction)
{
    if (!trans->is_active)
    {
        return;
    }

    if (trans->prop_mask & EGUI_STYLE_PROP_BG_COLOR)
    {
        trans->current.bg_color = egui_color_lerp(trans->from->bg_color, trans->to->bg_color, fraction);
        trans->current.bg_alpha = egui_alpha_lerp(trans->from->bg_alpha, trans->to->bg_alpha, fraction);
    }
    if (trans->prop_mask & EGUI_STYLE_PROP_BORDER)
    {
        trans->current.border_color = egui_color_lerp(trans->from->border_color, trans->to->border_color, fraction);
        trans->current.border_alpha = egui_alpha_lerp(trans->from->border_alpha, trans->to->border_alpha, fraction);
        trans->current.border_width = egui_dim_lerp(trans->from->border_width, trans->to->border_width, fraction);
    }
    if (trans->prop_mask & EGUI_STYLE_PROP_TEXT_COLOR)
    {
        trans->current.text_color = egui_color_lerp(trans->from->text_color, trans->to->text_color, fraction);
        trans->current.text_alpha = egui_alpha_lerp(trans->from->text_alpha, trans->to->text_alpha, fraction);
    }
    if (trans->prop_mask & EGUI_STYLE_PROP_RADIUS)
    {
        trans->current.radius = egui_dim_lerp(trans->from->radius, trans->to->radius, fraction);
    }
    if (trans->prop_mask & EGUI_STYLE_PROP_PADDING)
    {
        trans->current.pad_top = egui_dim_lerp(trans->from->pad_top, trans->to->pad_top, fraction);
        trans->current.pad_bottom = egui_dim_lerp(trans->from->pad_bottom, trans->to->pad_bottom, fraction);
        trans->current.pad_left = egui_dim_lerp(trans->from->pad_left, trans->to->pad_left, fraction);
        trans->current.pad_right = egui_dim_lerp(trans->from->pad_right, trans->to->pad_right, fraction);
    }

    /* Pointer properties: switch at midpoint (128) */
    if (fraction >= 128)
    {
        trans->current.shadow = trans->to->shadow;
        trans->current.text_font = trans->to->text_font;
        trans->current.bg_gradient = trans->to->bg_gradient;
    }

    if (fraction >= 255)
    {
        trans->is_active = 0;
    }
}

void egui_style_transition_finish(egui_style_transition_t *trans)
{
    if (trans->is_active && trans->to)
    {
        trans->current = *trans->to;
    }
    trans->is_active = 0;
}

const egui_style_t *egui_style_transition_get_current(const egui_style_transition_t *trans)
{
    if (trans->is_active)
    {
        return &trans->current;
    }
    return trans->to;
}
