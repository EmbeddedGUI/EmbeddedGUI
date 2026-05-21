#ifndef _EGUI_PROPERTY_H_
#define _EGUI_PROPERTY_H_

#include "egui_view.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_FUNCTION_PROPERTY_LITE

typedef enum egui_property_id
{
    EGUI_PROPERTY_X = 0,
    EGUI_PROPERTY_Y,
    EGUI_PROPERTY_WIDTH,
    EGUI_PROPERTY_HEIGHT,
    EGUI_PROPERTY_VISIBLE,
    EGUI_PROPERTY_ENABLED,
    EGUI_PROPERTY_CLICKABLE,
    EGUI_PROPERTY_ALPHA,
    EGUI_PROPERTY_TEXT,
    EGUI_PROPERTY_PADDING_LEFT,
    EGUI_PROPERTY_PADDING_RIGHT,
    EGUI_PROPERTY_PADDING_TOP,
    EGUI_PROPERTY_PADDING_BOTTOM,
    EGUI_PROPERTY_MARGIN_LEFT,
    EGUI_PROPERTY_MARGIN_RIGHT,
    EGUI_PROPERTY_MARGIN_TOP,
    EGUI_PROPERTY_MARGIN_BOTTOM,
} egui_property_id_t;

typedef enum egui_property_type
{
    EGUI_PROPERTY_TYPE_INT = 0,
    EGUI_PROPERTY_TYPE_U8,
    EGUI_PROPERTY_TYPE_STRING,
    EGUI_PROPERTY_TYPE_PTR,
} egui_property_type_t;

typedef struct egui_property_value
{
    egui_property_type_t type;
    union
    {
        int32_t i32;
        uint8_t u8;
        const char *str;
        void *ptr;
    } data;
} egui_property_value_t;

int egui_view_set_property(egui_view_t *self, egui_property_id_t id, const egui_property_value_t *value);
int egui_view_get_property(egui_view_t *self, egui_property_id_t id, egui_property_value_t *value);

#endif /* EGUI_CONFIG_FUNCTION_PROPERTY_LITE */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_PROPERTY_H_ */
