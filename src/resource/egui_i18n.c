#include <string.h>

#include "egui_i18n.h"
#include "core/egui_core.h"
#if EGUI_CONFIG_FUNCTION_EVENT_LITE
#include "core/egui_event.h"
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

static const char *s_empty_string = "";

void egui_i18n_init(egui_core_t *core, const egui_i18n_locale_t *locales, uint16_t locale_count)
{
    if (core == NULL)
    {
        return;
    }

    core->asset.i18n_locales = locales;
    core->asset.i18n_locale_count = locale_count;
    core->asset.i18n_current_locale = 0;
}

void egui_i18n_set_locale(egui_core_t *core, uint16_t locale_index)
{
    if (core != NULL && locale_index < core->asset.i18n_locale_count)
    {
        uint16_t old_locale = core->asset.i18n_current_locale;
        core->asset.i18n_current_locale = locale_index;
        if (old_locale != locale_index)
        {
#if EGUI_CONFIG_FUNCTION_EVENT_LITE
            egui_core_send_event_to_tree(core, EGUI_EVENT_LANGUAGE_CHANGED, &core->asset.i18n_current_locale);
#endif
            egui_core_force_refresh(core);
        }
    }
}

void egui_i18n_set_locale_by_code(egui_core_t *core, const char *locale_code)
{
    uint16_t i;

    if (core == NULL)
    {
        return;
    }

    if (locale_code == NULL)
    {
        uint16_t old_locale = core->asset.i18n_current_locale;
        core->asset.i18n_current_locale = 0;
        if (old_locale != 0)
        {
#if EGUI_CONFIG_FUNCTION_EVENT_LITE
            egui_core_send_event_to_tree(core, EGUI_EVENT_LANGUAGE_CHANGED, &core->asset.i18n_current_locale);
#endif
            egui_core_force_refresh(core);
        }
        return;
    }

    for (i = 0; i < core->asset.i18n_locale_count; i++)
    {
        if (core->asset.i18n_locales != NULL && core->asset.i18n_locales[i].locale_code != NULL &&
            strcmp(core->asset.i18n_locales[i].locale_code, locale_code) == 0)
        {
            uint16_t old_locale = core->asset.i18n_current_locale;
            core->asset.i18n_current_locale = i;
            if (old_locale != i)
            {
#if EGUI_CONFIG_FUNCTION_EVENT_LITE
                egui_core_send_event_to_tree(core, EGUI_EVENT_LANGUAGE_CHANGED, &core->asset.i18n_current_locale);
#endif
                egui_core_force_refresh(core);
            }
            return;
        }
    }

    /* Not found; fall back to default locale. */
    {
        uint16_t old_locale = core->asset.i18n_current_locale;
        core->asset.i18n_current_locale = 0;
        if (old_locale != 0)
        {
#if EGUI_CONFIG_FUNCTION_EVENT_LITE
            egui_core_send_event_to_tree(core, EGUI_EVENT_LANGUAGE_CHANGED, &core->asset.i18n_current_locale);
#endif
            egui_core_force_refresh(core);
        }
    }
}

uint16_t egui_i18n_get_locale(egui_core_t *core)
{
    if (core == NULL)
    {
        return 0;
    }

    return core->asset.i18n_current_locale;
}

const char *egui_i18n_get(egui_core_t *core, uint16_t string_id)
{
    const egui_i18n_locale_t *loc;

    if (core == NULL || core->asset.i18n_locales == NULL || core->asset.i18n_locale_count == 0)
    {
        return s_empty_string;
    }

    /* Try current locale first. */
    loc = &core->asset.i18n_locales[core->asset.i18n_current_locale];
    if (string_id < loc->count && loc->strings[string_id] != NULL && loc->strings[string_id][0] != '\0')
    {
        return loc->strings[string_id];
    }

    /* Fall back to default locale (index 0). */
    if (core->asset.i18n_current_locale != 0)
    {
        loc = &core->asset.i18n_locales[0];
        if (string_id < loc->count && loc->strings[string_id] != NULL)
        {
            return loc->strings[string_id];
        }
    }

    return s_empty_string;
}

uint16_t egui_i18n_get_locale_count(egui_core_t *core)
{
    if (core == NULL)
    {
        return 0;
    }

    return core->asset.i18n_locale_count;
}

const char *egui_i18n_get_locale_code(egui_core_t *core, uint16_t locale_index)
{
    if (core != NULL && locale_index < core->asset.i18n_locale_count && core->asset.i18n_locales != NULL &&
        core->asset.i18n_locales[locale_index].locale_code != NULL)
    {
        return core->asset.i18n_locales[locale_index].locale_code;
    }

    return s_empty_string;
}

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
