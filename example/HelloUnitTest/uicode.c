#include "egui.h"
#include "uicode.h"

#include "test/test_dirty_region.h"
#include "test/test_region.h"
#include "test/test_dlist.h"
#include "test/test_slist.h"
#include "test/test_ringbuffer.h"
#include "test/test_fixmath.h"
#include "test/test_common.h"
#include "test/test_view.h"
#include "test/test_view_group.h"
#include "test/test_linearlayout.h"
#include "test/test_interpolator.h"
#include "test/test_animation.h"
#include "test/test_view_layer.h"

void uicode_init_ui(void)
{
}

void uicode_create_ui(void)
{
    uicode_init_ui();

    // Run all test suites
    test_region_run();
    test_dlist_run();
    test_slist_run();
    test_ringbuffer_run();
    test_fixmath_run();
    test_common_run();
    test_view_run();
    test_view_group_run();
    test_linearlayout_run();
    test_interpolator_run();
    test_animation_run();
    test_dirty_region_run();
    test_view_layer_run();
}
