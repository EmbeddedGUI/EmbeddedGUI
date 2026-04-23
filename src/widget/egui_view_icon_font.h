#ifndef _EGUI_VIEW_ICON_FONT_H_
#define _EGUI_VIEW_ICON_FONT_H_

#include "config/egui_config.h"
#include "font/egui_font.h"
#include "resource/egui_resource.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Shared validity helpers used by icon/text-capable widgets before attempting font or label resolution. */
#define EGUI_VIEW_TEXT_VALID(_text)      ((_text) != NULL && (_text)[0] != '\0')
#define EGUI_VIEW_ICON_TEXT_VALID(_text) EGUI_VIEW_TEXT_VALID(_text)
/* Choose the explicit icon font when provided, otherwise fall back to an automatically sized Material Symbols font. */
#define EGUI_VIEW_ICON_FONT_RESOLVE(_font, _area_size, _icon16_max, _icon20_max)                                                                               \
    (((_font) != NULL) ? (_font) : egui_view_icon_font_get_auto((_area_size), (_icon16_max), (_icon20_max)))

/**
 * Resolve a stock Material Symbols font by available icon area.
 *
 * Widgets typically pass their content box size plus two threshold values that
 *
 * separate the 16px, 20px, and 24px built-in icon fonts.
 */
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

/**
 * Return the given built-in icon font only when automatic icon fallback is enabled.
 *
 * This lets widgets keep one code path whether the build includes
 * auto icon
 * font fallback or not.
 */
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
