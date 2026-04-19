#include <string.h>

#include "egui_i18n.h"
#include "core/egui_core.h"

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
        core->asset.i18n_current_locale = locale_index;
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
        core->asset.i18n_current_locale = 0;
        return;
    }

    for (i = 0; i < core->asset.i18n_locale_count; i++)
    {
        if (core->asset.i18n_locales != NULL &&
            core->asset.i18n_locales[i].locale_code != NULL &&
            strcmp(core->asset.i18n_locales[i].locale_code, locale_code) == 0)
        {
            core->asset.i18n_current_locale = i;
            return;
        }
    }

    /* Not found; fall back to default locale. */
    core->asset.i18n_current_locale = 0;
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
    if (string_id < loc->count && loc->strings[string_id] != NULL &&
        loc->strings[string_id][0] != '\0')
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
    if (core != NULL &&
        locale_index < core->asset.i18n_locale_count &&
        core->asset.i18n_locales != NULL &&
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
