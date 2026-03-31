#ifndef _EGUI_VIEW_ICON_FONT_H_
#define _EGUI_VIEW_ICON_FONT_H_

#include "core/egui_config.h"
#include "font/egui_font.h"
#include "resource/egui_resource.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline const egui_font_t *egui_view_icon_font_get_auto(egui_dim_t area_size, egui_dim_t icon16_max, egui_dim_t icon20_max)
{
#if EGUI_CONFIG_WIDGET_AUTO_ICON_FONT_FALLBACK
    if (area_size <= icon16_max)
    {
        return EGUI_FONT_ICON_MS_16;
    }
    if (area_size <= icon20_max)
    {
        return EGUI_FONT_ICON_MS_20;
    }
    return EGUI_FONT_ICON_MS_24;
#else
    (void)area_size;
    (void)icon16_max;
    (void)icon20_max;
    return NULL;
#endif
}

static inline const egui_font_t *egui_view_icon_font_get_auto_fixed(const egui_font_t *font)
{
#if EGUI_CONFIG_WIDGET_AUTO_ICON_FONT_FALLBACK
    return font;
#else
    (void)font;
    return NULL;
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_ICON_FONT_H_ */
