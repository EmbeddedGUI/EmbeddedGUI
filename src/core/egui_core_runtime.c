#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "egui_input.h"

void egui_polling_work(egui_core_t *core)
{
    egui_timer_polling_work(core);

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_input_polling_work(core);
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    if (!egui_input_check_key_idle(core))
    {
        egui_input_key_dispatch_work(core);
    }
#endif
}
