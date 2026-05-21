#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_msgbox.h"
#include "uicode_disp0.h"

#if EGUI_CONFIG_FUNCTION_MSGBOX

static int s_msgbox_cb_count;
static uint8_t s_msgbox_cb_index;
static const char *s_msgbox_cb_text;
static void *s_msgbox_cb_user_data;

static void test_msgbox_button_cb(egui_msgbox_t *msgbox, uint8_t index, const char *text, void *user_data)
{
    EGUI_UNUSED(msgbox);
    s_msgbox_cb_count++;
    s_msgbox_cb_index = index;
    s_msgbox_cb_text = text;
    s_msgbox_cb_user_data = user_data;
}

static void test_msgbox_init_text_and_button_click(void)
{
    egui_core_t *core = uicode_get_core();
    egui_activity_t activity;
    egui_msgbox_t msgbox;
    static const char *buttons[] = {"Cancel", "OK"};
    int user_data = 42;

    egui_activity_init(&activity, core);
    egui_msgbox_init(&msgbox, core);
    egui_msgbox_set_text(&msgbox, "Title", "Message");
    EGUI_TEST_ASSERT_TRUE(strcmp(egui_view_label_get_text(EGUI_VIEW_OF(&msgbox.title)), "Title") == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp(egui_view_label_get_text(EGUI_VIEW_OF(&msgbox.message)), "Message") == 0);

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_msgbox_set_buttons(&msgbox, buttons, 2, test_msgbox_button_cb, &user_data));
    EGUI_TEST_ASSERT_EQUAL_INT(2, msgbox.button_count);
    EGUI_TEST_ASSERT_TRUE(egui_msgbox_get_button(&msgbox, 1) == EGUI_VIEW_OF(&msgbox.buttons[1]));

    s_msgbox_cb_count = 0;
    s_msgbox_cb_index = 0;
    s_msgbox_cb_text = NULL;
    s_msgbox_cb_user_data = NULL;

    egui_msgbox_show(&msgbox, &activity);
    egui_view_perform_click(EGUI_VIEW_OF(&msgbox.buttons[1]));

    EGUI_TEST_ASSERT_EQUAL_INT(1, s_msgbox_cb_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_msgbox_cb_index);
    EGUI_TEST_ASSERT_TRUE(strcmp(s_msgbox_cb_text, "OK") == 0);
    EGUI_TEST_ASSERT_TRUE(s_msgbox_cb_user_data == &user_data);
    EGUI_TEST_ASSERT_EQUAL_INT(1, msgbox.dialog.is_need_finish);
}

static void test_msgbox_rejects_too_many_buttons(void)
{
    egui_core_t *core = uicode_get_core();
    egui_msgbox_t msgbox;
    const char *buttons[EGUI_CONFIG_MSGBOX_MAX_BUTTONS + 1] = {0};

    egui_msgbox_init(&msgbox, core);
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_msgbox_set_buttons(&msgbox, buttons, EGUI_CONFIG_MSGBOX_MAX_BUTTONS + 1, NULL, NULL));
}

#endif /* EGUI_CONFIG_FUNCTION_MSGBOX */

void test_msgbox_run(void)
{
    EGUI_TEST_SUITE_BEGIN(msgbox);

#if EGUI_CONFIG_FUNCTION_MSGBOX
    EGUI_TEST_RUN(test_msgbox_init_text_and_button_click);
    EGUI_TEST_RUN(test_msgbox_rejects_too_many_buttons);
#endif

    EGUI_TEST_SUITE_END();
}
