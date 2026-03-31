#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// TextBlock examples
static egui_view_textblock_t textblock_1;
static egui_view_textblock_t textblock_2;

// View params - use string literals directly
// textblock_1: Long single-line text automatically wraps and clamps to max lines.
EGUI_VIEW_TEXTBLOCK_PARAMS_INIT(textblock_1_params, 10, 10, 220, 72,
                                "TextBlock now wraps long strings automatically even when the source text does not contain explicit newline characters.",
                                EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);
// textblock_2: Long single-line text wraps into a scrollable paragraph with scrollbar.
EGUI_VIEW_TEXTBLOCK_PARAMS_INIT(
        textblock_2_params, 10, 96, 220, 86,
        "Scroller support stays optional so a paragraph can behave like a compact preview or a fully scrollable reading area depending on "
        "the screen and interaction needs of the widget.",
        EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);

void test_init_ui(void)
{
    // Init textblock_1: auto wrap + max lines, drag scroll disabled
    egui_view_textblock_init_with_params(EGUI_VIEW_OF(&textblock_1), &textblock_1_params);
    egui_view_textblock_set_align_type(EGUI_VIEW_OF(&textblock_1), EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP);
    egui_view_textblock_set_font_color(EGUI_VIEW_OF(&textblock_1), EGUI_THEME_TEXT, EGUI_ALPHA_100);
    egui_view_textblock_set_line_space(EGUI_VIEW_OF(&textblock_1), 4);
    egui_view_set_padding(EGUI_VIEW_OF(&textblock_1), 4, 4, 4, 4);
    egui_view_textblock_set_max_lines(EGUI_VIEW_OF(&textblock_1), 3);
    egui_view_textblock_set_scroll_enabled(EGUI_VIEW_OF(&textblock_1), 0);
    // Enable rounded border
    egui_view_textblock_set_border_enabled(EGUI_VIEW_OF(&textblock_1), 1);
    egui_view_textblock_set_border_radius(EGUI_VIEW_OF(&textblock_1), 6);
    egui_view_textblock_set_border_color(EGUI_VIEW_OF(&textblock_1), EGUI_THEME_PRIMARY_DARK);

    // Init textblock_2: auto wrap + vertical scrolling for overflow content
    egui_view_textblock_init_with_params(EGUI_VIEW_OF(&textblock_2), &textblock_2_params);
    egui_view_textblock_set_align_type(EGUI_VIEW_OF(&textblock_2), EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP);
    egui_view_textblock_set_font_color(EGUI_VIEW_OF(&textblock_2), EGUI_THEME_TEXT, EGUI_ALPHA_100);
    egui_view_textblock_set_line_space(EGUI_VIEW_OF(&textblock_2), 2);
    egui_view_set_padding(EGUI_VIEW_OF(&textblock_2), 4, 4, 4, 4);
    // Enable rounded border and scrollbar
    egui_view_textblock_set_border_enabled(EGUI_VIEW_OF(&textblock_2), 1);
    egui_view_textblock_set_border_radius(EGUI_VIEW_OF(&textblock_2), 6);
    egui_view_textblock_set_border_color(EGUI_VIEW_OF(&textblock_2), EGUI_THEME_PRIMARY_DARK);
#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    egui_view_textblock_set_scrollbar_enabled(EGUI_VIEW_OF(&textblock_2), 1);
#endif

    // Add to root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&textblock_1));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&textblock_2));

    // Keep explicit positions from params:
    // textblock_1: (10,10,220,72)
    // textblock_2: (10,96,220,86)
    // This guarantees visible top/side/between spacing.
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    if (action_index >= 1)
    {
        return false;
    }
    EGUI_SIM_SET_WAIT(p_action, 2000);
    return true;
}
#endif
