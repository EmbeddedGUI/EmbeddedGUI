#include <string.h>
#include "egui_i18n.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

static const egui_i18n_locale_t *s_locales = NULL;
static uint16_t s_locale_count = 0;
static uint16_t s_current_locale = 0;

static const char *s_empty_string = "";

void egui_i18n_init(const egui_i18n_locale_t *locales, uint16_t locale_count)
{
    s_locales = locales;
    s_locale_count = locale_count;
    s_current_locale = 0;
}

void egui_i18n_set_locale(uint16_t locale_index)
{
    if (locale_index < s_locale_count)
    {
        s_current_locale = locale_index;
    }
}

void egui_i18n_set_locale_by_code(const char *locale_code)
{
    uint16_t i;
    if (locale_code == NULL)
    {
        s_current_locale = 0;
        return;
    }
    for (i = 0; i < s_locale_count; i++)
    {
        if (s_locales[i].locale_code != NULL &&
            strcmp(s_locales[i].locale_code, locale_code) == 0)
        {
            s_current_locale = i;
            return;
        }
    }
    /* Not found — fall back to default */
    s_current_locale = 0;
}

uint16_t egui_i18n_get_locale(void)
{
    return s_current_locale;
}

const char *egui_i18n_get(uint16_t string_id)
{
    const egui_i18n_locale_t *loc;

    if (s_locales == NULL || s_locale_count == 0)
    {
        return s_empty_string;
    }

    /* Try current locale first */
    loc = &s_locales[s_current_locale];
    if (string_id < loc->count && loc->strings[string_id] != NULL &&
        loc->strings[string_id][0] != '\0')
    {
        return loc->strings[string_id];
    }

    /* Fall back to default locale (index 0) */
    if (s_current_locale != 0)
    {
        loc = &s_locales[0];
        if (string_id < loc->count && loc->strings[string_id] != NULL)
        {
            return loc->strings[string_id];
        }
    }

    return s_empty_string;
}

uint16_t egui_i18n_get_locale_count(void)
{
    return s_locale_count;
}

const char *egui_i18n_get_locale_code(uint16_t locale_index)
{
    if (locale_index < s_locale_count && s_locales[locale_index].locale_code != NULL)
    {
        return s_locales[locale_index].locale_code;
    }
    return s_empty_string;
}

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
