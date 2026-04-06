#ifndef _EGUI_VIEW_ICON_FONT_H_
#define _EGUI_VIEW_ICON_FONT_H_

#include "core/egui_config.h"
#include "font/egui_font.h"
#include "resource/egui_resource.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_TEXT_VALID(_text)      ((_text) != NULL && (_text)[0] != '\0')
#define EGUI_VIEW_ICON_TEXT_VALID(_text) EGUI_VIEW_TEXT_VALID(_text)
#define EGUI_VIEW_ICON_FONT_RESOLVE(_font, _area_size, _icon16_max, _icon20_max)                                                                               \
    (((_font) != NULL) ? (_font) : egui_view_icon_font_get_auto((_area_size), (_icon16_max), (_icon20_max)))

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
