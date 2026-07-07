#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include "egui_view_label.h"
#if EGUI_CONFIG_FUNCTION_VIEW_LABEL_COMPACT_ONLY || EGUI_CONFIG_FUNCTION_VIEW_LABEL_COMPACT_FALLBACK
#include "canvas/egui_canvas_compact.h"
#endif
#include "core/egui_core.h"
#include "font/egui_font.h"
#include "font/egui_font_std.h"

/*
 * The plain label is the simplest text widget in egui.
 * It borrows the caller's string pointer and delegates layout-aware drawing to the font/canvas
 * helpers.
 */

void egui_view_label_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_label_t);
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    if (local->text == NULL)
    {
        return;
    }

    egui_region_t region;
    egui_view_get_work_region(self, &region);

#if EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE
    if (local->long_mode == EGUI_LABEL_LONG_DOTS && local->font != NULL && local->font->api != NULL && local->font->api->get_str_size != NULL)
    {
        egui_dim_t text_w = 0, text_h = 0;
        egui_dim_t max_w = region.size.width;

        /* Measure full text width. */
        egui_font_get_str_size_with_canvas(local->font, canvas, local->text, 0, 0, &text_w, &text_h);

        if (text_w > max_w)
        {
            char buf[EGUI_CONFIG_LABEL_LONG_DOTS_BUF_SIZE];
            const char *src = local->text;
            egui_dim_t dots_w = 0, dots_h = 0;

            /* Measure "..." width so we know how much space to leave. */
            egui_font_get_str_size_with_canvas(local->font, canvas, "...", 0, 0, &dots_w, &dots_h);
            egui_dim_t avail_w = (egui_dim_t)(max_w - dots_w);

            /* Find the longest UTF-8 prefix that fits in avail_w. */
            int src_len = (int)strlen(src);
            int fit_len = 0; /* byte index of last fitting position */
            int i = 0;

            while (i < src_len)
            {
                /* Advance to end of current UTF-8 code-point (skip continuation bytes). */
                int char_end = i + 1;
                while (char_end < src_len && ((unsigned char)src[char_end] & 0xC0u) == 0x80u)
                {
                    char_end++;
                }

                /* Guard against buffer overflow. */
                if (char_end >= EGUI_CONFIG_LABEL_LONG_DOTS_BUF_SIZE - 3)
                {
                    break;
                }

                /* Null-terminate prefix and measure it. */
                memcpy(buf, src, (size_t)char_end);
                buf[char_end] = '\0';

                egui_dim_t w = 0, h = 0;
                egui_font_get_str_size_with_canvas(local->font, canvas, buf, 0, 0, &w, &h);

                if (w <= avail_w)
                {
                    fit_len = char_end;
                    i = char_end;
                }
                else
                {
                    break;
                }
            }

            /* Compose truncated string: prefix + "...". */
            memcpy(buf, src, (size_t)fit_len);
            buf[fit_len] = '.';
            buf[fit_len + 1] = '.';
            buf[fit_len + 2] = '.';
            buf[fit_len + 3] = '\0';

            egui_canvas_draw_text_in_rect_with_line_space(canvas, local->font, buf, &region, local->align_type, local->line_space, local->color, local->alpha);
            return;
        }
    }

#if EGUI_CONFIG_FUNCTION_LABEL_WORD_WRAP
    if (local->long_mode == EGUI_LABEL_LONG_WRAP && local->font != NULL && local->font->api != NULL && local->font->api->get_str_size != NULL)
    {
        /*
         * Word-wrap mode: scan the text left to right, building lines that fit
         * within region.size.width.  Lines are broken at space characters whenever
         * the current line would overflow; hard '\n' characters also force a break.
         * Up to EGUI_LABEL_WRAP_MAX_LINES lines are supported.  All lines are drawn
         * vertically centred inside the region.
         */
#define EGUI_LABEL_WRAP_MAX_LINES 16
#define EGUI_LABEL_WRAP_BUF       128

        const char *line_starts[EGUI_LABEL_WRAP_MAX_LINES];
        int line_lens[EGUI_LABEL_WRAP_MAX_LINES];
        int line_count = 0;
        egui_dim_t max_w = region.size.width;
        char tmp[EGUI_LABEL_WRAP_BUF];

        const char *p = local->text;
        const char *line_start = p;
        const char *last_break = NULL; /* last space position */
        int last_break_len = 0;

        while (line_count < EGUI_LABEL_WRAP_MAX_LINES)
        {
            if (*p == '\0' || *p == '\n')
            {
                /* Commit the current line. */
                int len = (int)(p - line_start);
                if (len > EGUI_LABEL_WRAP_BUF - 1)
                {
                    len = EGUI_LABEL_WRAP_BUF - 1;
                }
                line_starts[line_count] = line_start;
                line_lens[line_count] = len;
                line_count++;
                if (*p == '\n')
                {
                    p++;
                    line_start = p;
                    last_break = NULL;
                    continue;
                }
                break; /* '\0' */
            }

            if (*p == ' ')
            {
                last_break = p;
                last_break_len = (int)(p - line_start);
            }

            /* Advance past current UTF-8 codepoint. */
            int next = 1;
            while (((unsigned char)p[next] & 0xC0u) == 0x80u)
            {
                next++;
            }

            /* Measure from line_start to p+next. */
            int measure_len = (int)(p - line_start) + next;
            if (measure_len > EGUI_LABEL_WRAP_BUF - 1)
            {
                measure_len = EGUI_LABEL_WRAP_BUF - 1;
            }
            memcpy(tmp, line_start, (size_t)measure_len);
            tmp[measure_len] = '\0';

            egui_dim_t w = 0, h = 0;
            egui_font_get_str_size_with_canvas(local->font, canvas, tmp, 0, 0, &w, &h);

            if (w > max_w && (int)(p - line_start) > 0)
            {
                /* Wrap at last word break if available, else hard-wrap. */
                if (last_break != NULL)
                {
                    line_starts[line_count] = line_start;
                    line_lens[line_count] = last_break_len;
                    line_count++;
                    line_start = last_break + 1; /* skip the space */
                }
                else
                {
                    int len = (int)(p - line_start);
                    line_starts[line_count] = line_start;
                    line_lens[line_count] = len;
                    line_count++;
                    line_start = p;
                }
                last_break = NULL;
                /* Do not advance p; re-measure from new line_start. */
                continue;
            }

            p += next;
        }

        /* Measure single-line height. */
        egui_dim_t font_h = 0;
        egui_dim_t dummy_w = 0;
        egui_font_get_str_size_with_canvas(local->font, canvas, "A", 0, 0, &dummy_w, &font_h);
        if (font_h == 0)
        {
            goto wrap_done;
        }

        /* Vertical centering. */
        egui_dim_t line_h = (egui_dim_t)(font_h + local->line_space);
        egui_dim_t total_h = (egui_dim_t)(line_count > 0 ? (line_count - 1) * line_h + font_h : 0);
        egui_dim_t y = region.location.y + (region.size.height > total_h ? (region.size.height - total_h) / 2 : 0);

        {
            int li;
            for (li = 0; li < line_count; li++)
            {
                int copy_len = line_lens[li];
                if (copy_len <= 0)
                {
                    y = (egui_dim_t)(y + line_h);
                    continue;
                }
                if (copy_len > EGUI_LABEL_WRAP_BUF - 1)
                {
                    copy_len = EGUI_LABEL_WRAP_BUF - 1;
                }
                memcpy(tmp, line_starts[li], (size_t)copy_len);
                tmp[copy_len] = '\0';

                egui_dim_t lw = 0, lh = 0;
                egui_font_get_str_size_with_canvas(local->font, canvas, tmp, 0, 0, &lw, &lh);

                egui_dim_t x;
                if (local->align_type == EGUI_ALIGN_CENTER)
                {
                    x = region.location.x + (region.size.width - lw) / 2;
                }
                else if (local->align_type == EGUI_ALIGN_RIGHT)
                {
                    x = (egui_dim_t)(region.location.x + region.size.width - lw);
                }
                else
                {
                    x = region.location.x;
                }

                egui_canvas_draw_text(canvas, local->font, tmp, x, y, local->color, local->alpha);
                y = (egui_dim_t)(y + line_h);
            }
        }
    wrap_done:
        return;
    }
#endif /* EGUI_CONFIG_FUNCTION_LABEL_WORD_WRAP */

#endif /* EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE */

#if EGUI_CONFIG_FUNCTION_LABEL_RECOLOR
    if (local->is_recolor && local->font != NULL)
    {
        /*
         * Parse inline #RRGGBB text# segments and draw each span in its own color.
         * Syntax: "#RRGGBB span_text#" — a leading '#' followed by 6 hex digits, a
         * space, then the span text, terminated by a trailing '#'.
         * Text outside any tag uses the label's default color.
         *
         * Strategy: measure each span to compute x offsets, then call
         * egui_canvas_draw_text() with the appropriate color.
         */

        /* First pass: collect spans and measure total width. */
#define EGUI_RECOLOR_MAX_SPANS 16
        struct
        {
            const char *start;  /* pointer into text */
            int len;            /* byte length of span text (NOT the tag) */
            egui_color_t color; /* resolved color */
        } spans[EGUI_RECOLOR_MAX_SPANS];
        int span_count = 0;

        egui_dim_t total_w = 0;
        egui_dim_t font_h = 0;

        const char *p = local->text;
        while (*p && span_count < EGUI_RECOLOR_MAX_SPANS)
        {
            if (p[0] == '#' && p[1] != '\0' && p[1] != '#')
            {
                /* Try to parse "#RRGGBB " tag. */
                unsigned int r = 0, g = 0, b = 0;
                /* sscanf would pull in libc; use manual hex parse instead. */
                const char *q = p + 1;
                unsigned int vals[6] = {0};
                int ok = 1;
                int ki;
                for (ki = 0; ki < 6; ki++)
                {
                    char c = q[ki];
                    if (c >= '0' && c <= '9')
                    {
                        vals[ki] = (unsigned int)(c - '0');
                    }
                    else if (c >= 'A' && c <= 'F')
                    {
                        vals[ki] = (unsigned int)(c - 'A' + 10);
                    }
                    else if (c >= 'a' && c <= 'f')
                    {
                        vals[ki] = (unsigned int)(c - 'a' + 10);
                    }
                    else
                    {
                        ok = 0;
                        break;
                    }
                }
                if (ok && q[6] == ' ')
                {
                    r = (vals[0] << 4) | vals[1];
                    g = (vals[2] << 4) | vals[3];
                    b = (vals[4] << 4) | vals[5];

                    const char *text_start = q + 7; /* after "#RRGGBB " */
                    const char *text_end = text_start;
                    while (*text_end && *text_end != '#')
                    {
                        text_end++;
                    }

                    int seg_len = (int)(text_end - text_start);
                    if (seg_len > 0)
                    {
                        spans[span_count].start = text_start;
                        spans[span_count].len = seg_len;
                        spans[span_count].color = EGUI_COLOR_MAKE(r, g, b);

                        /* Measure span width. */
                        char tmp[64];
                        int copy_len = seg_len < 63 ? seg_len : 63;
                        int mi;
                        for (mi = 0; mi < copy_len; mi++)
                        {
                            tmp[mi] = text_start[mi];
                        }
                        tmp[copy_len] = '\0';
                        egui_dim_t sw = 0, sh = 0;
                        egui_font_get_str_size_with_canvas(local->font, canvas, tmp, 0, 0, &sw, &sh);
                        total_w += sw;
                        if (sh > font_h)
                        {
                            font_h = sh;
                        }

                        span_count++;
                    }

                    p = (*text_end == '#') ? text_end + 1 : text_end;
                    continue;
                }
            }

            /* Plain text segment: find end of plain run. */
            const char *seg_start = p;
            while (*p && *p != '#')
            {
                p++;
            }
            int seg_len = (int)(p - seg_start);
            if (seg_len > 0 && span_count < EGUI_RECOLOR_MAX_SPANS)
            {
                spans[span_count].start = seg_start;
                spans[span_count].len = seg_len;
                spans[span_count].color = local->color;

                char tmp[64];
                int copy_len = seg_len < 63 ? seg_len : 63;
                int mi;
                for (mi = 0; mi < copy_len; mi++)
                {
                    tmp[mi] = seg_start[mi];
                }
                tmp[copy_len] = '\0';
                egui_dim_t sw = 0, sh = 0;
                egui_font_get_str_size_with_canvas(local->font, canvas, tmp, 0, 0, &sw, &sh);
                total_w += sw;
                if (sh > font_h)
                {
                    font_h = sh;
                }
                span_count++;
            }
        }

        if (span_count > 0 && font_h > 0)
        {
            /* Compute starting x based on alignment. */
            egui_dim_t start_x = region.location.x;
            if (local->align_type == EGUI_ALIGN_CENTER)
            {
                start_x = region.location.x + (region.size.width - total_w) / 2;
            }
            else if (local->align_type == EGUI_ALIGN_RIGHT)
            {
                start_x = region.location.x + region.size.width - total_w;
            }

            egui_dim_t y = region.location.y + (region.size.height - font_h) / 2;

            /* Second pass: draw each span. */
            char tmp[64];
            egui_dim_t cur_x = start_x;
            int si;
            for (si = 0; si < span_count; si++)
            {
                int copy_len = spans[si].len < 63 ? spans[si].len : 63;
                int mi;
                for (mi = 0; mi < copy_len; mi++)
                {
                    tmp[mi] = spans[si].start[mi];
                }
                tmp[copy_len] = '\0';
                egui_canvas_draw_text(canvas, local->font, tmp, cur_x, y, spans[si].color, local->alpha);

                egui_dim_t sw = 0, sh = 0;
                egui_font_get_str_size_with_canvas(local->font, canvas, tmp, 0, 0, &sw, &sh);
                cur_x = (egui_dim_t)(cur_x + sw);
            }
        }
        return;
    }
#endif /* EGUI_CONFIG_FUNCTION_LABEL_RECOLOR */

#if EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE
    if (local->font != NULL && local->font->api != NULL && local->font->api->get_str_size != NULL && local->letter_space > 0)
    {
        /*
         * Draw characters one by one, inserting letter_space pixels between each.
         * Alignment is computed from the total measured width including spacing.
         * Only handles single-line text (no '\n' splitting); multi-line falls
         * through to the standard path.
         */
        egui_dim_t text_w = 0, font_h = 0;
        egui_font_get_str_size_with_canvas(local->font, canvas, local->text, 0, 0, &text_w, &font_h);

        /* Count UTF-8 code-points to compute extra spacing. */
        int cp_count = 0;
        {
            const char *q = local->text;
            while (*q)
            {
                if (((unsigned char)*q & 0xC0u) != 0x80u)
                {
                    cp_count++;
                }
                q++;
            }
        }

        egui_dim_t total_w = (egui_dim_t)(text_w + (cp_count > 1 ? (cp_count - 1) * (int)local->letter_space : 0));

        egui_dim_t x;
        if (local->align_type == EGUI_ALIGN_CENTER)
        {
            x = region.location.x + (region.size.width - total_w) / 2;
        }
        else if (local->align_type == EGUI_ALIGN_RIGHT)
        {
            x = (egui_dim_t)(region.location.x + region.size.width - total_w);
        }
        else
        {
            x = region.location.x;
        }
        egui_dim_t y = region.location.y + (region.size.height - font_h) / 2;

        const char *p = local->text;
        while (*p)
        {
            /* Collect one UTF-8 codepoint. */
            char buf[8];
            int len = 1;
            buf[0] = *p;
            while (p[len] && ((unsigned char)p[len] & 0xC0u) == 0x80u && len < 7)
            {
                buf[len] = p[len];
                len++;
            }
            buf[len] = '\0';

            egui_dim_t cw = 0, ch = 0;
            egui_font_get_str_size_with_canvas(local->font, canvas, buf, 0, 0, &cw, &ch);
            egui_canvas_draw_text(canvas, local->font, buf, x, y, local->color, local->alpha);
            x = (egui_dim_t)(x + cw + local->letter_space);
            p += len;
        }
        return;
    }
#endif /* EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE */

#if EGUI_CONFIG_FUNCTION_VIEW_LABEL_COMPACT_ONLY
    egui_canvas_compact_text_draw(canvas, local->text, &region, local->align_type, local->color, local->alpha);
#else
    if (local->font != NULL)
    {
        egui_canvas_draw_text_in_rect_with_line_space(canvas, local->font, local->text, &region, local->align_type, local->line_space, local->color,
                                                      local->alpha);
    }
#if EGUI_CONFIG_FUNCTION_VIEW_LABEL_COMPACT_FALLBACK
    else
    {
        // Keep ASCII-only labels working when callers intentionally omit a font.
        egui_canvas_compact_text_draw(canvas, local->text, &region, local->align_type, local->color, local->alpha);
    }
#endif
#endif
}

void egui_view_label_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_label_t);
    if (local->font == font)
    {
        return;
    }
    local->font = font;
    egui_view_invalidate(self);
}

const egui_font_t *egui_view_label_get_font(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_label_t);
    return local->font;
}

void egui_view_label_set_font_with_std_height(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_label_t);
    egui_dim_t height;

    if (font == NULL)
    {
        egui_view_label_set_font(self, NULL);
        return;
    }

    height = EGUI_FONT_STD_GET_FONT_HEIGHT(font);

    // This convenience helper keeps the view height aligned with the font's standard line height.
    egui_view_label_set_font(self, font);
    if (height == self->region.size.height)
    {
        return;
    }
    egui_view_set_size(self, self->region.size.width, height);
    egui_view_invalidate(self);
}

void egui_view_label_set_font_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha)
{
    EGUI_LOCAL_INIT(egui_view_label_t);
    if ((local->color.full == color.full) && (local->alpha == alpha))
    {
        return;
    }
    local->color = color;
    local->alpha = alpha;
    egui_view_invalidate(self);
}

egui_color_t egui_view_label_get_font_color(egui_view_t *self)
{
    if (self == NULL)
    {
        egui_color_t zero;
        zero.full = 0;
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_label_t);
    return local->color;
}

egui_alpha_t egui_view_label_get_alpha(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_label_t);
    return local->alpha;
}

void egui_view_label_set_align_type(egui_view_t *self, uint8_t align_type)
{
    EGUI_LOCAL_INIT(egui_view_label_t);
    if (local->align_type == align_type)
    {
        return;
    }
    local->align_type = align_type;
    egui_view_invalidate(self);
}

uint8_t egui_view_label_get_align_type(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_label_t);
    return local->align_type;
}

void egui_view_label_set_text(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_label_t);
    local->text = text;
    egui_view_invalidate(self);
}

const char *egui_view_label_get_text(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_label_t);
    return local->text;
}

#if EGUI_CONFIG_FUNCTION_LABEL_TEXT_FMT
void egui_view_label_set_text_fmt(egui_view_t *self, const char *fmt, ...)
{
    va_list args;
    if (self == NULL || fmt == NULL)
    {
        return;
    }
    EGUI_LOCAL_INIT(egui_view_label_t);
    va_start(args, fmt);
    vsnprintf(local->fmt_buf, EGUI_CONFIG_LABEL_FMT_BUF_SIZE, fmt, args);
    va_end(args);
    local->fmt_buf[EGUI_CONFIG_LABEL_FMT_BUF_SIZE - 1] = '\0';
    local->text = local->fmt_buf;
    egui_view_invalidate(self);
}
#endif /* EGUI_CONFIG_FUNCTION_LABEL_TEXT_FMT */

void egui_view_label_set_line_space(egui_view_t *self, egui_dim_t line_space)
{
    EGUI_LOCAL_INIT(egui_view_label_t);
    local->line_space = line_space;
    egui_view_invalidate(self);
}

egui_dim_t egui_view_label_get_line_space(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_label_t);
    return local->line_space;
}

#if EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE
void egui_view_label_set_letter_space(egui_view_t *self, egui_dim_t letter_space)
{
    if (self == NULL)
    {
        return;
    }
    EGUI_LOCAL_INIT(egui_view_label_t);
    local->letter_space = letter_space;
    egui_view_invalidate(self);
}

egui_dim_t egui_view_label_get_letter_space(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_label_t);
    return local->letter_space;
}
#endif /* EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE */

int egui_view_label_get_str_size(egui_view_t *self, const void *string, egui_dim_t *width, egui_dim_t *height)
{
    EGUI_LOCAL_INIT(egui_view_label_t);

#if EGUI_CONFIG_FUNCTION_VIEW_LABEL_COMPACT_ONLY
    {
        egui_canvas_compact_text_layout_t layout;

        if (egui_canvas_compact_text_measure((const char *)string, self->region.size.width, self->region.size.height, &layout))
        {
            if (width != NULL)
            {
                *width = layout.width;
            }
            if (height != NULL)
            {
                *height = layout.height;
            }
            return 0;
        }
    }
#else
    if (local->font != NULL && local->font->api != NULL && local->font->api->get_str_size != NULL)
    {
        if (egui_font_get_str_size_with_core(local->font, egui_view_get_core(self), string, 0, 0, width, height) == 0)
        {
            return 0;
        }
    }
#if EGUI_CONFIG_FUNCTION_VIEW_LABEL_COMPACT_FALLBACK
    else
    {
        egui_canvas_compact_text_layout_t layout;

        if (egui_canvas_compact_text_measure((const char *)string, self->region.size.width, self->region.size.height, &layout))
        {
            if (width != NULL)
            {
                *width = layout.width;
            }
            if (height != NULL)
            {
                *height = layout.height;
            }
            return 0;
        }
    }
#else
    else
    {
        EGUI_UNUSED(self);
        EGUI_UNUSED(string);
    }
#endif

    if (width != NULL)
    {
        *width = 0;
    }
    if (height != NULL)
    {
        *height = 0;
    }

    return -1;
#endif
}

int egui_view_label_get_str_size_with_padding(egui_view_t *self, const void *string, egui_dim_t *width, egui_dim_t *height)
{
    int ret = egui_view_label_get_str_size(self, string, width, height);

    if (ret != 0)
    {
        return ret;
    }

    // This version reports the outer widget size needed to contain the measured text.
    *width += self->padding.left + self->padding.right;
    *height += self->padding.top + self->padding.bottom;

    return 0;
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_label_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_label_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_label_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_label_t);

    // Initialize the base view first so invalidation and shared view behavior are available.
    egui_view_init(self, core);
    // Replace the generic draw callback with the label-specific implementation.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_label_t);

    // Defaults describe a centered text widget with no borrowed font/text yet.
    local->line_space = 0;

#if EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE
    local->letter_space = 0;
#endif

    local->align_type = EGUI_ALIGN_CENTER;
    local->alpha = EGUI_ALPHA_100;
    local->color = EGUI_THEME_TEXT_PRIMARY;

    local->font = NULL;
    local->text = NULL;

#if EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE
    local->long_mode = EGUI_LABEL_LONG_CLIP;
#endif

#if EGUI_CONFIG_FUNCTION_LABEL_RECOLOR
    local->is_recolor = 0;
#endif

    egui_view_set_view_name(self, "egui_view_label");
}

void egui_view_label_apply_params(egui_view_t *self, const egui_view_label_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_label_t);

    // Params directly seed the borrowed text/font pointers and visual style.
    self->region = params->region;

    local->text = params->text;
    local->font = params->font;
    local->color = params->color;
    local->alpha = params->alpha;
    local->align_type = params->align_type;

    egui_view_invalidate(self);
}

void egui_view_label_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_label_params_t *params)
{
    egui_view_label_init(self, core);
    egui_view_label_apply_params(self, params);
}

#if EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE
/**
 * @brief Set the text overflow mode.  Use EGUI_LABEL_LONG_CLIP or EGUI_LABEL_LONG_DOTS.
 */
void egui_view_label_set_long_mode(egui_view_t *self, uint8_t mode)
{
    if (self == NULL)
    {
        return;
    }
    EGUI_LOCAL_INIT(egui_view_label_t);
    if (local->long_mode == mode)
    {
        return;
    }
    local->long_mode = mode;
    egui_view_invalidate(self);
}

/**
 * @brief Return the current text overflow mode.
 */
uint8_t egui_view_label_get_long_mode(egui_view_t *self)
{
    if (self == NULL)
    {
        return EGUI_LABEL_LONG_CLIP;
    }
    EGUI_LOCAL_INIT(egui_view_label_t);
    return local->long_mode;
}
#endif /* EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE */

#if EGUI_CONFIG_FUNCTION_LABEL_RECOLOR
void egui_view_label_set_recolor(egui_view_t *self, uint8_t is_recolor)
{
    if (self == NULL)
    {
        return;
    }
    EGUI_LOCAL_INIT(egui_view_label_t);
    if (local->is_recolor == is_recolor)
    {
        return;
    }
    local->is_recolor = is_recolor;
    egui_view_invalidate(self);
}

uint8_t egui_view_label_get_recolor(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_label_t);
    return local->is_recolor;
}
#endif /* EGUI_CONFIG_FUNCTION_LABEL_RECOLOR */
