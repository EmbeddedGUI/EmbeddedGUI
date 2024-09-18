#include "egui.h"
#include <stdlib.h>
#include "uicode.h"
#include "egui_view_page_test.h"

// views in root
static egui_view_viewpage_cache_t viewpage_1;

static int index = 0;
static int test_get_index(void)
{
    return index++;
}

void* on_page_load_listener(egui_view_t *self, int current_page_index)
{
    EGUI_LOG_INF("on_page_load_listener, current_page_index: %d\r\n", current_page_index);
    egui_view_page_test_t *p_page = (egui_view_page_test_t *)egui_api_malloc(sizeof(egui_view_page_test_t));
    egui_view_page_test_init((egui_view_t *)p_page);
    egui_view_page_test_set_index((egui_view_t *)p_page, current_page_index);
    return p_page;
}

void on_page_free_listener(egui_view_t *self, int current_page_index, egui_view_t *page)
{
    EGUI_LOG_INF("on_page_free_listener, current_page_index: %d\r\n", current_page_index);
    egui_api_free(page);
}

void test_init_ui(void)
{
    // Init all views
    // viewpage_1
    egui_view_viewpage_cache_init((egui_view_t *)&viewpage_1);
    egui_view_set_position((egui_view_t *)&viewpage_1, 0, 0);
    egui_view_viewpage_cache_set_size((egui_view_t *)&viewpage_1, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_viewpage_cache_set_child_total_cnt((egui_view_t *)&viewpage_1, 10);
    egui_view_viewpage_cache_set_on_page_load_listener((egui_view_t *)&viewpage_1, on_page_load_listener);
    egui_view_viewpage_cache_set_on_page_free_listener((egui_view_t *)&viewpage_1, on_page_free_listener);

    egui_view_viewpage_cache_set_current_page((egui_view_t *)&viewpage_1, 0);
    
    // egui_view_viewpage_cache_set_current_page((egui_view_t *)&viewpage_1, 7);

    // Add To Root
    egui_core_add_user_root_view((egui_view_t *)&viewpage_1);
}
