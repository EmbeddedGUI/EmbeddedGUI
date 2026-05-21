#include "egui_property.h"

#if EGUI_CONFIG_FUNCTION_PROPERTY_LITE

#include "egui_view_label.h"

static int egui_property_get_int(const egui_property_value_t *value, int32_t *out)
{
    if (value == NULL || out == NULL)
    {
        return -1;
    }
    if (value->type == EGUI_PROPERTY_TYPE_INT)
    {
        *out = value->data.i32;
        return 0;
    }
    if (value->type == EGUI_PROPERTY_TYPE_U8)
    {
        *out = value->data.u8;
        return 0;
    }
    return -1;
}

int egui_view_set_property(egui_view_t *self, egui_property_id_t id, const egui_property_value_t *value)
{
    int32_t i32 = 0;

    if (self == NULL || value == NULL)
    {
        return -1;
    }

    switch (id)
    {
    case EGUI_PROPERTY_X:
        if (egui_property_get_int(value, &i32) != 0)
        {
            return -1;
        }
        egui_view_set_position(self, (egui_dim_t)i32, egui_view_get_y(self));
        return 0;
    case EGUI_PROPERTY_Y:
        if (egui_property_get_int(value, &i32) != 0)
        {
            return -1;
        }
        egui_view_set_position(self, egui_view_get_x(self), (egui_dim_t)i32);
        return 0;
    case EGUI_PROPERTY_WIDTH:
        if (egui_property_get_int(value, &i32) != 0)
        {
            return -1;
        }
        egui_view_set_size(self, (egui_dim_t)i32, egui_view_get_height(self));
        return 0;
    case EGUI_PROPERTY_HEIGHT:
        if (egui_property_get_int(value, &i32) != 0)
        {
            return -1;
        }
        egui_view_set_size(self, egui_view_get_width(self), (egui_dim_t)i32);
        return 0;
    case EGUI_PROPERTY_VISIBLE:
        if (egui_property_get_int(value, &i32) != 0)
        {
            return -1;
        }
        egui_view_set_visible(self, i32 != 0);
        return 0;
    case EGUI_PROPERTY_ENABLED:
        if (egui_property_get_int(value, &i32) != 0)
        {
            return -1;
        }
        egui_view_set_enable(self, i32 != 0);
        return 0;
    case EGUI_PROPERTY_CLICKABLE:
        if (egui_property_get_int(value, &i32) != 0)
        {
            return -1;
        }
        egui_view_set_clickable(self, i32 != 0);
        return 0;
    case EGUI_PROPERTY_ALPHA:
        if (egui_property_get_int(value, &i32) != 0)
        {
            return -1;
        }
        if (i32 < 0)
        {
            i32 = 0;
        }
        else if (i32 > EGUI_ALPHA_100)
        {
            i32 = EGUI_ALPHA_100;
        }
        egui_view_set_alpha(self, (egui_alpha_t)i32);
        return 0;
    case EGUI_PROPERTY_TEXT:
        if (value->type != EGUI_PROPERTY_TYPE_STRING)
        {
            return -1;
        }
        egui_view_label_set_text(self, value->data.str);
        return 0;
    case EGUI_PROPERTY_PADDING_LEFT:
        if (egui_property_get_int(value, &i32) != 0)
        {
            return -1;
        }
        egui_view_set_padding(self, (egui_dim_margin_padding_t)i32, egui_view_get_padding_right(self), egui_view_get_padding_top(self),
                              egui_view_get_padding_bottom(self));
        return 0;
    case EGUI_PROPERTY_PADDING_RIGHT:
        if (egui_property_get_int(value, &i32) != 0)
        {
            return -1;
        }
        egui_view_set_padding(self, egui_view_get_padding_left(self), (egui_dim_margin_padding_t)i32, egui_view_get_padding_top(self),
                              egui_view_get_padding_bottom(self));
        return 0;
    case EGUI_PROPERTY_PADDING_TOP:
        if (egui_property_get_int(value, &i32) != 0)
        {
            return -1;
        }
        egui_view_set_padding(self, egui_view_get_padding_left(self), egui_view_get_padding_right(self), (egui_dim_margin_padding_t)i32,
                              egui_view_get_padding_bottom(self));
        return 0;
    case EGUI_PROPERTY_PADDING_BOTTOM:
        if (egui_property_get_int(value, &i32) != 0)
        {
            return -1;
        }
        egui_view_set_padding(self, egui_view_get_padding_left(self), egui_view_get_padding_right(self), egui_view_get_padding_top(self),
                              (egui_dim_margin_padding_t)i32);
        return 0;
    case EGUI_PROPERTY_MARGIN_LEFT:
        if (egui_property_get_int(value, &i32) != 0)
        {
            return -1;
        }
        egui_view_set_margin(self, (egui_dim_margin_padding_t)i32, egui_view_get_margin_right(self), egui_view_get_margin_top(self),
                             egui_view_get_margin_bottom(self));
        return 0;
    case EGUI_PROPERTY_MARGIN_RIGHT:
        if (egui_property_get_int(value, &i32) != 0)
        {
            return -1;
        }
        egui_view_set_margin(self, egui_view_get_margin_left(self), (egui_dim_margin_padding_t)i32, egui_view_get_margin_top(self),
                             egui_view_get_margin_bottom(self));
        return 0;
    case EGUI_PROPERTY_MARGIN_TOP:
        if (egui_property_get_int(value, &i32) != 0)
        {
            return -1;
        }
        egui_view_set_margin(self, egui_view_get_margin_left(self), egui_view_get_margin_right(self), (egui_dim_margin_padding_t)i32,
                             egui_view_get_margin_bottom(self));
        return 0;
    case EGUI_PROPERTY_MARGIN_BOTTOM:
        if (egui_property_get_int(value, &i32) != 0)
        {
            return -1;
        }
        egui_view_set_margin(self, egui_view_get_margin_left(self), egui_view_get_margin_right(self), egui_view_get_margin_top(self),
                             (egui_dim_margin_padding_t)i32);
        return 0;
    default:
        break;
    }

    return -1;
}

int egui_view_get_property(egui_view_t *self, egui_property_id_t id, egui_property_value_t *value)
{
    if (self == NULL || value == NULL)
    {
        return -1;
    }

    switch (id)
    {
    case EGUI_PROPERTY_X:
        value->type = EGUI_PROPERTY_TYPE_INT;
        value->data.i32 = egui_view_get_x(self);
        return 0;
    case EGUI_PROPERTY_Y:
        value->type = EGUI_PROPERTY_TYPE_INT;
        value->data.i32 = egui_view_get_y(self);
        return 0;
    case EGUI_PROPERTY_WIDTH:
        value->type = EGUI_PROPERTY_TYPE_INT;
        value->data.i32 = egui_view_get_width(self);
        return 0;
    case EGUI_PROPERTY_HEIGHT:
        value->type = EGUI_PROPERTY_TYPE_INT;
        value->data.i32 = egui_view_get_height(self);
        return 0;
    case EGUI_PROPERTY_VISIBLE:
        value->type = EGUI_PROPERTY_TYPE_U8;
        value->data.u8 = (uint8_t)egui_view_get_visible(self);
        return 0;
    case EGUI_PROPERTY_ENABLED:
        value->type = EGUI_PROPERTY_TYPE_U8;
        value->data.u8 = (uint8_t)egui_view_get_enable(self);
        return 0;
    case EGUI_PROPERTY_CLICKABLE:
        value->type = EGUI_PROPERTY_TYPE_U8;
        value->data.u8 = (uint8_t)egui_view_get_clickable(self);
        return 0;
    case EGUI_PROPERTY_ALPHA:
        value->type = EGUI_PROPERTY_TYPE_U8;
        value->data.u8 = egui_view_get_alpha(self);
        return 0;
    case EGUI_PROPERTY_TEXT:
        value->type = EGUI_PROPERTY_TYPE_STRING;
        value->data.str = egui_view_label_get_text(self);
        return 0;
    case EGUI_PROPERTY_PADDING_LEFT:
        value->type = EGUI_PROPERTY_TYPE_INT;
        value->data.i32 = egui_view_get_padding_left(self);
        return 0;
    case EGUI_PROPERTY_PADDING_RIGHT:
        value->type = EGUI_PROPERTY_TYPE_INT;
        value->data.i32 = egui_view_get_padding_right(self);
        return 0;
    case EGUI_PROPERTY_PADDING_TOP:
        value->type = EGUI_PROPERTY_TYPE_INT;
        value->data.i32 = egui_view_get_padding_top(self);
        return 0;
    case EGUI_PROPERTY_PADDING_BOTTOM:
        value->type = EGUI_PROPERTY_TYPE_INT;
        value->data.i32 = egui_view_get_padding_bottom(self);
        return 0;
    case EGUI_PROPERTY_MARGIN_LEFT:
        value->type = EGUI_PROPERTY_TYPE_INT;
        value->data.i32 = egui_view_get_margin_left(self);
        return 0;
    case EGUI_PROPERTY_MARGIN_RIGHT:
        value->type = EGUI_PROPERTY_TYPE_INT;
        value->data.i32 = egui_view_get_margin_right(self);
        return 0;
    case EGUI_PROPERTY_MARGIN_TOP:
        value->type = EGUI_PROPERTY_TYPE_INT;
        value->data.i32 = egui_view_get_margin_top(self);
        return 0;
    case EGUI_PROPERTY_MARGIN_BOTTOM:
        value->type = EGUI_PROPERTY_TYPE_INT;
        value->data.i32 = egui_view_get_margin_bottom(self);
        return 0;
    default:
        break;
    }

    return -1;
}

#endif /* EGUI_CONFIG_FUNCTION_PROPERTY_LITE */
